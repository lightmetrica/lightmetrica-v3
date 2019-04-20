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
    Transform globalTransform;  // Global transform of the primitive
    int primitive;              // Primitive node index
};

using InstancedScene = std::vector<FlattenedPrimitiveNode>;

}

// ----------------------------------------------------------------------------

class Accel_Embree_Instanced final : public Accel {
private:
    RTCDevice device_ = nullptr;
    RTCScene scene_ = nullptr;
    std::vector<InstancedScene> instancedScenes_;

public:
    Accel_Embree_Instanced() {
        device_ = rtcNewDevice("");
        handleEmbreeError(nullptr, rtcGetDeviceError(device_));
        rtcSetDeviceErrorFunction(device_, handleEmbreeError, nullptr);
    }

    ~Accel_Embree_Instanced() {
        if (!device_) {
            rtcReleaseDevice(device_);
        }
    }

private:
    void reset() {
        if (scene_) {
            rtcReleaseScene(scene_);
            scene_ = nullptr;
        }
        instancedScenes_.clear();
    }

public:
    virtual void build(const Scene& scene) override {
        exception::ScopedDisableFPEx guard_;

        reset();
        scene_ = rtcNewScene(device_);

        // TBA

        rtcCommitScene(scene_);
    }

    virtual std::optional<Hit> intersect(Ray ray, Float tmin, Float tmax) const override {
        exception::ScopedDisableFPEx guard_;

        // TBA
        return {};
    }

};

LM_COMP_REG_IMPL(Accel_Embree_Instanced, "accel::embreeinstanced");

LM_NAMESPACE_END(LM_NAMESPACE)
