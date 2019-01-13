/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/material.h>
#include <lm/scene.h>
#include <lm/texture.h>
#include <lm/json.h>
#include <lm/user.h>
#include <lm/serial.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Material_Diffuse : public Material {
private:
    Vec3 Kd_;
    const Texture* mapKd_ = nullptr;

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(Kd_, mapKd_);
    }

public:
    virtual bool construct(const Json& prop) override {
        mapKd_ = getAsset<Texture>(prop, "mapKd");
        if (!mapKd_) {
            Kd_ = json::valueOr<Vec3>(prop, "Kd", Vec3(1_f));
        }
        return true;
    }

    virtual bool isSpecular(const SurfacePoint& sp) const override {
        LM_UNUSED(sp);
        return false;
    }

    virtual std::optional<RaySample> sampleRay(Rng& rng, const SurfacePoint& sp, Vec3 wi) const {
        const auto[n, u, v] = sp.orthonormalBasis(wi);
        const auto Kd = mapKd_ ? mapKd_->eval(sp.t) : Kd_;
        const auto d = math::sampleCosineWeighted(rng);
        return RaySample(
            sp,
            u*d.x + v*d.y + n*d.z,
            Kd
        );
    }

    virtual std::optional<Vec3> reflectance(const SurfacePoint& sp) const {
        return mapKd_ ? mapKd_->eval(sp.t) : Kd_;
    }

    virtual Float pdf(const SurfacePoint& sp, Vec3 wi, Vec3 wo) const {
        return sp.opposite(wi, wo) ? 0_f : 1_f / Pi;
    }

    virtual Vec3 eval(const SurfacePoint& sp, Vec3 wi, Vec3 wo) const {
        if (sp.opposite(wi, wo)) {
            return {};
        }
        const auto a = (mapKd_ && mapKd_->hasAlpha()) ? mapKd_->evalAlpha(sp.t) : 1_f;
        return (mapKd_ ? mapKd_->eval(sp.t) : Kd_) * (a / Pi);
    }
};

LM_COMP_REG_IMPL(Material_Diffuse, "material::diffuse");

LM_NAMESPACE_END(LM_NAMESPACE)
