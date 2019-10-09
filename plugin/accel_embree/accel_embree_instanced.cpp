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
    Transform globalTransform;      // Global transform of the flattened node
    int nodeIndex;                  // Index of (unflattened) scene node
    int flattenedSceneIndex;        // Index of flattened scene only used for InstancedScene type
};

using FlattenedScene = std::vector<FlattenedSceneNode>;

}

// ------------------------------------------------------------------------------------------------

class Accel_Embree_Instanced final : public Accel {
private:
    RTCDevice device_ = nullptr;
    RTCScene scene_ = nullptr;
    std::vector<FlattenedScene> flattenedScenes_;    // Flattened scenes (index 0: root)

public:
    Accel_Embree_Instanced() {
        device_ = rtcNewDevice("");
        handleEmbreeError(nullptr, rtcGetDeviceError(device_));
        rtcSetDeviceErrorFunction(device_, handleEmbreeError, nullptr);
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
        flattenedScenes_.clear();
    }

public:
    virtual void build(const Scene& scene) override {
        using namespace std::placeholders;
        exception::ScopedDisableFPEx guard_;
        reset();

        // ----------------------------------------------------------------------------------------

        // Flatten the scene with single-level instance group
        LM_INFO("Flattening scene");
        std::unordered_map<int, int> nodeToFlattenedSceneMap;     // Node index -> flattened scene index
        using VisitSceneNodeFunc = std::function<void(const SceneNode, Mat4, int, bool)>;
        VisitSceneNodeFunc visitSceneNode = [&](const SceneNode& node, Mat4 globalTransform, int flattenedSceneIndex, bool ignoreInstanceGroup) {
            // Primitive node type
            if (node.type == SceneNodeType::Primitive) {
                // Record flatten primitive
                auto& flattenedScene = flattenedScenes_.at(flattenedSceneIndex);
                const int flattenedNodeIndex = int(flattenedScene.size());
                flattenedScene.push_back({
                    FlattenedSceneNodeType::Primitive,
                    flattenedNodeIndex,
                    Transform(globalTransform),
                    node.index
                });

                return;
            }

            // ------------------------------------------------------------------------------------

            // Group node type
            if (node.type == SceneNodeType::Group) {
                // Apply local transform
                Mat4 M = globalTransform;
                if (node.group.localTransform) {
                    M *= *node.group.localTransform;
                }

                // Instance group
                if (!ignoreInstanceGroup && node.group.instanced) {
                    // Get index of child flattened scene
                    int childFlattenedSceneIndex = -1;
                    if (auto it = nodeToFlattenedSceneMap.find(node.index); it != nodeToFlattenedSceneMap.end()) {
                        // Get recorded flattened scene index
                        childFlattenedSceneIndex = it->second;
                    }
                    else {
                        // Create a new flattened scene if not available
                        childFlattenedSceneIndex = int(flattenedScenes_.size());
                        nodeToFlattenedSceneMap[node.index] = childFlattenedSceneIndex;
                        flattenedScenes_.emplace_back();
                        scene.visitNode(node.index, std::bind(visitSceneNode, _1, Mat4(1_f), childFlattenedSceneIndex, true));
                    }

                    // Add flattened node
                    auto& flattenedScene = flattenedScenes_.at(flattenedSceneIndex);
                    const int flattenedNodeIndex = int(flattenedScene.size());
                    flattenedScene.push_back({
                        FlattenedSceneNodeType::InstancedScene,
                        flattenedNodeIndex,
                        Transform(globalTransform),
                        node.index,
                        childFlattenedSceneIndex
                    });

                    return;
                }

                // Normal group
                for (int child : node.group.children) {
                    scene.visitNode(child, std::bind(visitSceneNode, _1, M, flattenedSceneIndex, ignoreInstanceGroup));
                }

                return;
            }

            // ------------------------------------------------------------------------------------

            LM_UNREACHABLE();
        };
        flattenedScenes_.emplace_back();
        scene.visitNode(0, std::bind(visitSceneNode, _1, Mat4(1_f), 0, false));

        // ----------------------------------------------------------------------------------------

        // Traverse the flattened scene and create embree scene
        // Process from backward because the instanced scene must be created prior to the scene.
        LM_INFO("Building");
        std::vector<RTCScene> rtcscenes(flattenedScenes_.size());
        for (int i = int(flattenedScenes_.size())-1; i >= 0; i--) {
            const auto& fscene = flattenedScenes_.at(i);

            // Create a new embree scene
            auto& rtcscene = rtcscenes[i];
            rtcscene = rtcNewScene(device_);

            // Create triangle meshes
            for (const auto& fnode : fscene) {
                // Primitive
                if (fnode.type == FlattenedSceneNodeType::Primitive) {
                    // Get unflattened primitive node
                    const auto& node = scene.nodeAt(fnode.nodeIndex);
                    assert(node.type == SceneNodeType::Primitive);
                    if (!node.primitive.mesh) {
                        continue;
                    }

                    // Create embree's triangle mesh
                    auto geom = rtcNewGeometry(device_, RTC_GEOMETRY_TYPE_TRIANGLE);
                    const int numTriangles = node.primitive.mesh->numTriangles();
                    auto* vs = (glm::vec3*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(glm::vec3), numTriangles * 3);
                    auto* fs = (glm::uvec3*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(glm::uvec3), numTriangles);
                    node.primitive.mesh->foreachTriangle([&](int face, const Mesh::Tri& tri) {
                        const auto p1 = fnode.globalTransform.M * Vec4(tri.p1.p, 1_f);
                        const auto p2 = fnode.globalTransform.M * Vec4(tri.p2.p, 1_f);
                        const auto p3 = fnode.globalTransform.M * Vec4(tri.p3.p, 1_f);
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
                    rtcSetGeometryInstancedScene(inst, rtcscenes.at(fnode.flattenedSceneIndex));
                    glm::mat4 M(fnode.globalTransform.M);
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
        const auto [M, nodeIndex] = [&]() -> std::tuple<Mat4, int> {
            const auto instID = rayhit.hit.instID[0];
            if (instID != RTC_INVALID_GEOMETRY_ID) {
                const auto& fn1 = flattenedScenes_.at(0).at(instID);
                const auto& fn2 = flattenedScenes_.at(fn1.flattenedSceneIndex).at(rayhit.hit.geomID);
                return { fn1.globalTransform.M * fn2.globalTransform.M, fn2.nodeIndex };
            }
            else {
                const auto& fn = flattenedScenes_.at(0).at(rayhit.hit.geomID);
                return { fn.globalTransform.M, fn.nodeIndex };
            }
        }();

        // ----------------------------------------------------------------------------------------

        // Store hit information
        return Hit{
            Float(rayhit.ray.tfar),
            Vec2(Float(rayhit.hit.u), Float(rayhit.hit.v)),
            Transform(M),
            nodeIndex,
            int(rayhit.hit.primID)
        };
    }
};

LM_COMP_REG_IMPL(Accel_Embree_Instanced, "accel::embreeinstanced");

LM_NAMESPACE_END(LM_NAMESPACE)
