/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/accel.h>
#include <lm/scene.h>
#include <lm/exception.h>
#include <lm/logger.h>
#include <embree3/rtcore.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

namespace {
void handleEmbreeError(void* userPtr, RTCError code, const char* str = nullptr) {
    if (code == RTC_ERROR_NONE) {
        return;
    }

    std::string codestr;
    switch (code) {
        case RTC_ERROR_UNKNOWN:           { codestr = "RTC_ERROR_UNKNOWN"; break; }
        case RTC_ERROR_INVALID_ARGUMENT:  { codestr = "RTC_ERROR_INVALID_ARGUMENT"; break; }
        case RTC_ERROR_INVALID_OPERATION: { codestr = "RTC_ERROR_INVALID_OPERATION"; break; }
        case RTC_ERROR_OUT_OF_MEMORY:     { codestr = "RTC_ERROR_OUT_OF_MEMORY"; break; }
        case RTC_ERROR_UNSUPPORTED_CPU:   { codestr = "RTC_ERROR_UNSUPPORTED_CPU"; break; }
        case RTC_ERROR_CANCELLED:         { codestr = "RTC_ERROR_CANCELLED"; break; }
        default:                          { codestr = "Invalid error code"; break; }
    }

    LM_ERROR("Embree error [code='{}']", codestr);
    if (str) {
        LM_INDENT();
        LM_ERROR(str);
    }

    throw std::runtime_error(codestr);
}
}

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

public:
    Accel_Embree() {
        device_ = rtcNewDevice("");
        handleEmbreeError(nullptr, rtcGetDeviceError(device_));
        rtcSetDeviceErrorFunction(device_, handleEmbreeError, nullptr);
    }

    ~Accel_Embree() {
        if (!device_) {
            rtcReleaseDevice(device_);
        }
    }

public:
    virtual bool construct(const Json& prop) override {
        scene_ = rtcNewScene(device_);

        // 

        rtcCommitScene(scene_);
    }

    virtual void build(const Scene& scene) override {
        LM_UNUSED(scene);
    }

    virtual std::optional<Hit> intersect(Ray ray, Float tmin, Float tmax) const override {
        LM_UNUSED(ray, tmin, tmax);
        return {};
    }
};

LM_COMP_REG_IMPL(Accel_Embree, "accel::embree");

LM_NAMESPACE_END(LM_NAMESPACE)
