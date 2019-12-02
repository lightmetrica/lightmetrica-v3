/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/accel.h>
#include <lm/scene.h>
#include <lm/mesh.h>
#include <lm/exception.h>
#include "embree.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

namespace {  

struct FlattenedPrimitiveNode {
    Transform global_transform;  // Global transform of the primitive
    int primitive;              // Primitive node index
};

}

// ------------------------------------------------------------------------------------------------

/*
\rst
.. function:: accel::embree

   Acceleration structure with Embree library.
\endrst
*/
class Accel_Embree final : public Accel {
private:
    RTCDevice device_ = nullptr;
    RTCScene scene_ = nullptr;
    std::vector<FlattenedPrimitiveNode> flattened_nodes_;

public:
    Accel_Embree() {
        device_ = rtcNewDevice("");
        handleEmbreeError(nullptr, rtcGetDeviceError(device_));
        rtcSetDeviceErrorFunction(device_, handleEmbreeError, nullptr);
    }

    ~Accel_Embree() {
        if (scene_) {
            rtcReleaseScene(scene_);
        }
        if (device_) {
            rtcReleaseDevice(device_);
        }
    }

private:
    void reset() {
        if (scene_) {
            rtcReleaseScene(scene_);
            scene_ = nullptr;
        }
        flattened_nodes_.clear();
    }

public:
    virtual void build(const Scene& scene) override {
        exception::ScopedDisableFPEx guard_;

        reset();
        scene_ = rtcNewScene(device_);

        // Flatten the scene graph and setup geometries
        LM_INFO("Flattening scene");
        scene.traverse_primitive_nodes([&](const SceneNode& node, Mat4 global_transform) {
            if (node.type != SceneNodeType::Primitive) {
                return;
            }
            if (!node.primitive.mesh) {
                return;
            }

            // Record flattened primitive
            const int flattenNodeIndex = int(flattened_nodes_.size());
            flattened_nodes_.push_back({ Transform(global_transform), node.index });

            // Create triangle mesh
            auto geom = rtcNewGeometry(device_, RTC_GEOMETRY_TYPE_TRIANGLE);
            const int num_triangles = node.primitive.mesh->num_triangles();
            auto* vs = (glm::vec3*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(glm::vec3), num_triangles*3);
            auto* fs = (glm::uvec3*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(glm::uvec3), num_triangles);
            node.primitive.mesh->foreach_triangle([&](int face, const Mesh::Tri& tri) {
                const auto p1 = global_transform * Vec4(tri.p1.p, 1_f);
                const auto p2 = global_transform * Vec4(tri.p2.p, 1_f);
                const auto p3 = global_transform * Vec4(tri.p3.p, 1_f);
                vs[3*face  ] = glm::vec3(p1);
                vs[3*face+1] = glm::vec3(p2);
                vs[3*face+2] = glm::vec3(p3);
                fs[face][0] = 3*face;
                fs[face][1] = 3*face+1;
                fs[face][2] = 3*face+2;
            });
            rtcCommitGeometry(geom);
            rtcAttachGeometryByID(scene_, geom, flattenNodeIndex);
            rtcReleaseGeometry(geom);
        });

        LM_INFO("Building");
        rtcCommitScene(scene_);
    }

    virtual std::optional<Hit> intersect(Ray ray, Float tmin, Float tmax) const override {
        exception::ScopedDisableFPEx guard_;

        RTCIntersectContext context;
        rtcInitIntersectContext(&context);

        // Setup ray
        RTCRayHit rayhit;
        rayhit.ray.org_x = float(ray.o.x);
        rayhit.ray.org_y = float(ray.o.y);
        rayhit.ray.org_z = float(ray.o.z);
        rayhit.ray.tnear = float(tmin);
        rayhit.ray.dir_x = float(ray.d.x);
        rayhit.ray.dir_y = float(ray.d.y);
        rayhit.ray.dir_z = float(ray.d.z);
        rayhit.ray.time = 0.f;
        rayhit.ray.tfar = float(tmax);
        rayhit.hit.primID = RTC_INVALID_GEOMETRY_ID;
        rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
        rayhit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;
        
        // Intersection query
        rtcIntersect1(scene_, &context, &rayhit);
        if (rayhit.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
            // No intersection
            return {};
        }
        
        // Store hit information
        const auto& fn = flattened_nodes_.at(rayhit.hit.geomID);
        return Hit{
            Float(rayhit.ray.tfar),
            Vec2(Float(rayhit.hit.u), Float(rayhit.hit.v)),
            fn.global_transform,
            fn.primitive,
            int(rayhit.hit.primID)
        };
    }
};

LM_COMP_REG_IMPL(Accel_Embree, "accel::embree");

LM_NAMESPACE_END(LM_NAMESPACE)
