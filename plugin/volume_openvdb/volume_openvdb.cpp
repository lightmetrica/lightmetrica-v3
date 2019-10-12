/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/volume.h>
#include <lm/core.h>
#include <vdbloader.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Volume_OpenVDBScalar : public Volume {
private:
    VDBLoaderContext context_;
    Float scale_;
    Bound bound_;
    Float maxScalar_;

public:
    Volume_OpenVDBScalar() {
        vdbloaderSetErrorFunc(nullptr, [](void*, int errorCode, const char* message) {
            const auto errStr = [&]() -> std::string {
                if (errorCode == VDBLOADER_ERROR_INVALID_CONTEXT)
                    return "INVALID_CONTEXT";
                else if (errorCode == VDBLOADER_ERROR_INVALID_ARGUMENT)
                    return "INVALID_ARGUMENT";
                else if (errorCode == VDBLOADER_ERROR_UNKNOWN)
                    return "UNKNOWN";
                LM_UNREACHABLE_RETURN();
            }();
            LM_ERROR("vdbloader error: {} [type='{}']", message, errStr);
        });
        context_ = vdbloaderCreateContext();
    }

    ~Volume_OpenVDBScalar() {
        vdbloaderReleaseContext(context_);
    }

public:
    virtual void construct(const Json& prop) override {
        // Load VDB file
        const auto path = json::value<std::string>(prop, "path");
        LM_INFO("Opening OpenVDB file [path='{}']", path);
        if (!vdbloaderLoadVDBFile(context_, path.c_str())) {
            LM_THROW_EXCEPTION(Error::IOError, "Failed to load OpenVDB file [path='{}']", path);
        }

        // Density scale
        scale_ = json::value<Float>(prop, "scale", 1_f);

        // Bound
        const auto b = vdbloaderGetBound(context_);
        bound_.mi = Vec3(b.min.x, b.min.y, b.min.z);
        bound_.ma = Vec3(b.max.x, b.max.y, b.max.z);

        // Maximum density
        maxScalar_ = vbdloaderGetMaxScalar(context_) * scale_;
    }

    virtual Bound bound() const override {
        return bound_;
    }

    virtual Float maxScalar() const override {
        return maxScalar_;
    }

    virtual bool hasScalar() const override {
        return true;
    }

    virtual Float evalScalar(Vec3 p) const override {
        const auto d = vbdloaderEvalScalar(context_, VDBLoaderFloat3{ p.x, p.y, p.z });
        return d * scale_;
    }

    virtual bool hasColor() const override {
        return false;
    }

    virtual void march(Ray ray, Float tmin, Float tmax, Float marchStep, const RaymarchFunc& raymarchFunc) const override {
        exception::ScopedDisableFPEx guard_;
        const void* f = reinterpret_cast<const void*>(&raymarchFunc);
        vdbloaderMarchVolume(
            context_,
            VDBLoaderFloat3{ ray.o.x, ray.o.y, ray.o.z },
            VDBLoaderFloat3{ ray.d.x, ray.d.y, ray.d.z },
            tmin, tmax, marchStep, const_cast<void*>(f),
            [](void* user, double t) -> bool {
                const void* f = const_cast<const void*>(user);
                const RaymarchFunc& raymarchFunc = *reinterpret_cast<const RaymarchFunc*>(f);
                return raymarchFunc(t);
            });
    }
};

LM_COMP_REG_IMPL(Volume_OpenVDBScalar, "volume::openvdb_scalar");

LM_NAMESPACE_END(LM_NAMESPACE)
