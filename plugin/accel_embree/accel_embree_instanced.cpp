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

enum class FlattenedSceneNodeType {
    Primitive,
    InstancedScene,
};

struct FlattenedSceneNode {
    FlattenedSceneNodeType type;    // Type
    int index;                      // Index of flattened node
    Transform global_transform;     // Global transform of the flattened node
    int node_index;                 // Index of (unflattened) scene node
    int flattened_scene_index;      // Index of flattened scene only used for InstancedScene type
};

using FlattenedScene = std::vector<FlattenedSceneNode>;

}

// ------------------------------------------------------------------------------------------------

class Accel_Embree_Instanced final : public Accel {
private:
    RTCDevice device_ = nullptr;
    RTCScene scene_ = nullptr;
    RTCBuildArguments settings_;
    RTCSceneFlags sf_;
    std::vector<FlattenedScene> flattened_scenes_;    // Flattened scenes (index 0: root)

public:
    
     virtual void construct(const Json& prop) override {
        settings_ = rtcDefaultBuildArguments();
        from_json(prop,settings_);

        from_json(prop,sf_);

        Json j;
        to_json(j,settings_);
        LM_DEBUG(j.dump());
        j.clear();
        
        to_json(j,sf_);
        LM_DEBUG(j.dump() );
        
        }

    Accel_Embree_Instanced() {
        device_ = rtcNewDevice("");
        handle_embree_error(nullptr, rtcGetDeviceError(device_));
        rtcSetDeviceErrorFunction(device_, handle_embree_error, nullptr);
    }

    ~Accel_Embree_Instanced() {
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
        flattened_scenes_.clear();
    }

public:
    virtual void build(const Scene& scene) override {
        using namespace std::placeholders;
        exception::ScopedDisableFPEx guard_;
        reset();

        // ----------------------------------------------------------------------------------------

        // Flatten the scene with single-level instance group
        LM_INFO("Flattening scene");
		// Node index -> flattened scene index
        std::unordered_map<int, int> node_to_flattened_scene_map;
        using VisitSceneNodeFunc = std::function<void(const SceneNode, Mat4, int, bool)>;
        VisitSceneNodeFunc visit_scene_node = [&](const SceneNode& node, Mat4 global_transform, int flattened_scene_index, bool ignoreInstanceGroup) {
            // Primitive node type
            if (node.type == SceneNodeType::Primitive) {
                // Record flatten primitive
                auto& flattened_scene = flattened_scenes_.at(flattened_scene_index);
                const int flattened_node_index = int(flattened_scene.size());
                flattened_scene.push_back({
                    FlattenedSceneNodeType::Primitive,
                    flattened_node_index,
                    Transform(global_transform),
                    node.index
                });

                return;
            }

            // ------------------------------------------------------------------------------------

            // Group node type
            if (node.type == SceneNodeType::Group) {
                // Apply local transform
                Mat4 M = global_transform;
                if (node.group.local_transform) {
                    M *= *node.group.local_transform;
                }

                // Instance group
                if (!ignoreInstanceGroup && node.group.instanced) {
                    // Get index of child flattened scene
                    int child_flattened_scene_index = -1;
                    if (auto it = node_to_flattened_scene_map.find(node.index); it != node_to_flattened_scene_map.end()) {
                        // Get recorded flattened scene index
                        child_flattened_scene_index = it->second;
                    }
                    else {
                        // Create a new flattened scene if not available
                        child_flattened_scene_index = int(flattened_scenes_.size());
                        node_to_flattened_scene_map[node.index] = child_flattened_scene_index;
                        flattened_scenes_.emplace_back();
                        scene.visit_node(node.index, std::bind(visit_scene_node, _1, Mat4(1_f), child_flattened_scene_index, true));
                    }

                    // Add flattened node
                    auto& flattened_scene = flattened_scenes_.at(flattened_scene_index);
                    const int flattened_node_index = int(flattened_scene.size());
                    flattened_scene.push_back({
                        FlattenedSceneNodeType::InstancedScene,
                        flattened_node_index,
                        Transform(global_transform),
                        node.index,
                        child_flattened_scene_index
                    });

                    return;
                }

                // Normal group
                for (int child : node.group.children) {
                    scene.visit_node(child, std::bind(visit_scene_node, _1, M, flattened_scene_index, ignoreInstanceGroup));
                }

                return;
            }

            // ------------------------------------------------------------------------------------

            LM_UNREACHABLE();
        };
        flattened_scenes_.emplace_back();
        scene.visit_node(0, std::bind(visit_scene_node, _1, Mat4(1_f), 0, false));

        // ----------------------------------------------------------------------------------------

        // Traverse the flattened scene and create embree scene
        // Process from backward because the instanced scene must be created prior to the scene.
        LM_INFO("Building");
        std::vector<RTCScene> rtcscenes(flattened_scenes_.size());
        for (int i = int(flattened_scenes_.size())-1; i >= 0; i--) {
            const auto& fscene = flattened_scenes_.at(i);

            // Create a new embree scene
            auto& rtcscene = rtcscenes[i];
            rtcscene = rtcNewScene(device_);
            rtcSetSceneFlags(rtcscene, sf_);
            rtcSetSceneBuildQuality(rtcscene, settings_.buildQuality);
            rtc_AT_SetNextBVHArguments(scene_, settings_);
    
            // Create triangle meshes
            for (const auto& fnode : fscene) {
                // Primitive
                if (fnode.type == FlattenedSceneNodeType::Primitive) {
                    // Get unflattened primitive node
                    const auto& node = scene.node_at(fnode.node_index);
                    assert(node.type == SceneNodeType::Primitive);
                    if (!node.primitive.mesh) {
                        continue;
                    }

                    // Create embree's triangle mesh
                    auto geom = rtcNewGeometry(device_, RTC_GEOMETRY_TYPE_TRIANGLE);
                    const int num_triangles = node.primitive.mesh->num_triangles();
                    auto* vs = (glm::vec3*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(glm::vec3), num_triangles * 3);
                    auto* fs = (glm::uvec3*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(glm::uvec3), num_triangles);
                    node.primitive.mesh->foreach_triangle([&](int face, const Mesh::Tri& tri) {
                        const auto p1 = fnode.global_transform.M * Vec4(tri.p1.p, 1_f);
                        const auto p2 = fnode.global_transform.M * Vec4(tri.p2.p, 1_f);
                        const auto p3 = fnode.global_transform.M * Vec4(tri.p3.p, 1_f);
                        vs[3 * face] = glm::vec3(p1);
                        vs[3 * face + 1] = glm::vec3(p2);
                        vs[3 * face + 2] = glm::vec3(p3);
                        fs[face][0] = 3 * face;
                        fs[face][1] = 3 * face + 1;
                        fs[face][2] = 3 * face + 2;
                    });
                    rtcCommitGeometry(geom);
                    rtcAttachGeometryByID(rtcscene, geom, fnode.index);
                    rtcReleaseGeometry(geom);
                }

                // Instanced scene
                else if (fnode.type == FlattenedSceneNodeType::InstancedScene) {
                    // Index must to be zero
                    assert(i == 0);

                    // Create instanced geometry
                    auto inst = rtcNewGeometry(device_, RTC_GEOMETRY_TYPE_INSTANCE);
                    rtcSetGeometryInstancedScene(inst, rtcscenes.at(fnode.flattened_scene_index));
                    glm::mat4 M(fnode.global_transform.M);
                    rtcSetGeometryTransform(inst, 0, RTC_FORMAT_FLOAT4X4_COLUMN_MAJOR, &M[0].x);
                    rtcCommitGeometry(inst);
                    rtcAttachGeometryByID(rtcscene, inst, fnode.index);
                    rtcReleaseGeometry(inst);
                }
            }

            // Commit the embree scene
            rtcCommitScene(rtcscene);
        }

        // Keep the root embree scene
        scene_ = rtcscenes[0];
    }

    virtual std::optional<Hit> intersect(Ray ray, Float tmin, Float tmax) const override {
        exception::ScopedDisableFPEx guard_;

        RTCIntersectContext context;
        rtcInitIntersectContext(&context);

        // ----------------------------------------------------------------------------------------

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

        // ----------------------------------------------------------------------------------------

        // Intersection query
        rtcIntersect1(scene_, &context, &rayhit);
        if (rayhit.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
            // No intersection
            return {};
        }

        // ----------------------------------------------------------------------------------------

        // Get global transform and (unflattened) node index
        // corresponding to the intersected (instanced) geometry
        const auto [M, node_index] = [&]() -> std::tuple<Mat4, int> {
            const auto instID = rayhit.hit.instID[0];
            if (instID != RTC_INVALID_GEOMETRY_ID) {
                const auto& fn1 = flattened_scenes_.at(0).at(instID);
                const auto& fn2 = flattened_scenes_.at(fn1.flattened_scene_index).at(rayhit.hit.geomID);
                return { fn1.global_transform.M * fn2.global_transform.M, fn2.node_index };
            }
            else {
                const auto& fn = flattened_scenes_.at(0).at(rayhit.hit.geomID);
                return { fn.global_transform.M, fn.node_index };
            }
        }();

        // ----------------------------------------------------------------------------------------

        // Store hit information
        return Hit{
            Float(rayhit.ray.tfar),
            Vec2(Float(rayhit.hit.u), Float(rayhit.hit.v)),
            Transform(M),
            node_index,
            int(rayhit.hit.primID)
        };
    }
};

LM_COMP_REG_IMPL(Accel_Embree_Instanced, "accel::embreeinstanced");

LM_NAMESPACE_END(LM_NAMESPACE)
