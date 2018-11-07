/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/material.h>
#include <lm/scene.h>
#include <lm/texture.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Material_Diffuse : public Material {
private:
    Vec3 Kd_;
    const Texture* mapKd_ = nullptr;

public:
    virtual bool construct(const Json& prop) override {
        mapKd_ = parent()->underlying<Texture>(prop, "mapKd");
        if (!mapKd_) {
            Kd_ = castFromJson<Vec3>(prop["Kd"]);
        }
        return true;
    }

    virtual bool isSpecular(const SurfacePoint& sp) const override {
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

    virtual Vec3 reflectance(const SurfacePoint& sp) const {
        return mapKd_ ? mapKd_->eval(sp.t) : Kd_;
    }
};

LM_COMP_REG_IMPL(Material_Diffuse, "material::diffuse");

LM_NAMESPACE_END(LM_NAMESPACE)
