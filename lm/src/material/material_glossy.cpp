/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/material.h>
#include <lm/scene.h>
#include <lm/json.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Material_Glossy : public Material {
private:
    Vec3 Ks_;        // Specular reflectance
    Float ax_, ay_;  // Roughness (anisotropic)

public:
    virtual bool construct(const Json& prop) override {
        Ks_ = castFromJson<Vec3>(prop["Ks"]);
        ax_ = prop["ax"];
        ay_ = prop["ay"];
        return true;
    }

    virtual bool isSpecular(const SurfacePoint& sp) const override {
        LM_UNUSED(sp);
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

    virtual Vec3 reflectance(const SurfacePoint& sp) const {
        LM_UNUSED(sp);
        return Ks_;
    }

    virtual Float pdf(const SurfacePoint& sp, Vec3 wi, Vec3 wo) const {
        if (sp.opposite(wi, wo)) {
            return 0_f;
        }
        const auto wh = glm::normalize(wi + wo);
        const auto [n, u, v] = sp.orthonormalBasis(wi);
        return GGX_D(wh, u, v, n) * glm::dot(wh, n)
            / (4_f * glm::dot(wo, wh) * glm::dot(wo, n));
    }

    virtual Vec3 eval(const SurfacePoint& sp, Vec3 wi, Vec3 wo) const {
        if (sp.opposite(wi, wo)) {
            return {};
        }
        const auto wh = glm::normalize(wi + wo);
        const auto [n, u, v] = sp.orthonormalBasis(wi);
        const auto Fr = Ks_ + (1_f - Ks_) * std::pow(1_f - dot(wo, wh), 5_f);
        return Ks_ * Fr * (GGX_D(wh, u, v, n) * GGX_G(wi, wo, u, v, n)
            / (4_f * dot(wi, n) * dot(wo, n)));
    }

private:
    // Normal distribution of anisotropic GGX
    Float GGX_D(Vec3 wh, Vec3 u, Vec3 v, Vec3 n) const {
        return 1_f / (Pi*ax_*ay_*math::sq(math::sq(glm::dot(wh, u) / ax_) +
            math::sq(glm::dot(wh, v) / ay_) + math::sq(glm::dot(wh, n))));
    }

    // Smith's G term correspoinding to the anisotropic GGX
    Float GGX_G(Vec3 wi, Vec3 wo, Vec3 u, Vec3 v, Vec3 n) const {
        const auto G1 = [&](Vec3 w) {
            const auto c = glm::dot(w, n);
            const auto s = std::sqrt(1_f - c * c);
            const auto cp = glm::dot(w, u) / s;
            const auto cs = glm::dot(w, v) / s;
            const auto a2 = math::sq(cp * ax_) + math::sq(cs * ay_);
            return c == 0_f ? 0_f : 2_f / (1_f + std::sqrt(1_f + a2 * math::sq(s / c)));
        };
        return G1(wi) * G1(wo);
    }
};

LM_COMP_REG_IMPL(Material_Glossy, "material::glossy");

LM_NAMESPACE_END(LM_NAMESPACE)
