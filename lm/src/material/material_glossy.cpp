/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/material.h>
#include <lm/scene.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Material_Glossy : public Material {
private:
    Vec3 Ks_;        // Specular reflectance
    Float ax_, ay_;  // Roughness (anisotropic)

public:
    virtual bool construct(const Json& prop) override {
        Ks_ = prop["Ks"];
        ax_ = prop["ax"];
        ay_ = prop["ay"];
        return true;
    }

    virtual bool isSpecular() const override {
        return false;
    }

    virtual std::optional<RaySample> sampleRay(Rng& rng, const SurfacePoint& sp, Vec3 wi) const {
        const auto [n, u, v] = sp.orthonormalBasis(wi);
        const auto u1 = rng.u() * 2_f * Pi;
        const auto u2 = rng.u();
        const auto wh = glm::normalize(glm::sqrt(u2/(1_f-u2))*(ax_*cos(u1)*u+ay_*sin(u1)*v)+n);
        const auto wo = math::reflection(wi, wh);
        if (sp.opposite(wi, wo)) {
            return {};
        }
        return RaySample(
            sp,
            wo,
            eval(sp, wi, wo) / pdf(sp, wi, wo)
        );
    }
};

LM_COMP_REG_IMPL(Material_Glossy, "material::glossy");

LM_NAMESPACE_END(LM_NAMESPACE)
