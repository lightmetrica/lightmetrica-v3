/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/core.h>
#include <lm/material.h>
#include <lm/surface.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Material_Glossy final : public Material {
private:
    Vec3 Ks_;        // Specular reflectance
    Float ax_, ay_;  // Roughness (anisotropic)

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(Ks_, ax_, ay_);
    }

public:
    virtual void construct(const Json& prop) override {
        Ks_ = json::value<Vec3>(prop, "Ks");
        ax_ = json::value<Float>(prop, "ax");
        ay_ = json::value<Float>(prop, "ay");
    }

public:
    virtual bool is_specular(const PointGeometry&, int) const override {
        return false;
    }

    virtual std::optional<MaterialDirectionSample> sample_direction(Rng& rng, const PointGeometry& geom, Vec3 wi) const override {
        const auto [n, u, v] = geom.orthonormal_basis(wi);
        const auto u1 = rng.u() * 2_f * Pi;
        const auto u2 = rng.u();
        const auto wh = glm::normalize(math::safe_sqrt(u2/(1_f-u2))*(ax_*glm::cos(u1)*u+ay_*glm::sin(u1)*v)+n);
        const auto wo = math::reflection(wi, wh);
        if (geom.opposite(wi, wo)) {
            return {};
        }
        const auto f = eval(geom, {}, wi, wo);
        const auto p = pdf_direction(geom, {}, wi, wo);
        return MaterialDirectionSample{
            wo,
            SurfaceComp::DontCare,
            f / p
        };
    }

    virtual std::optional<Vec3> sample_direction_given_comp(Rng& rng, const PointGeometry& geom, int, Vec3 wi) const override {
        return sample_direction(rng, geom, wi)->wo;
    }

    virtual std::optional<Vec3> reflectance(const PointGeometry&, int) const override {
        return Ks_;
    }

    virtual Float pdf_direction(const PointGeometry& geom, int, Vec3 wi, Vec3 wo) const override {
        if (geom.opposite(wi, wo)) {
            return 0_f;
        }
        const auto wh = glm::normalize(wi + wo);
        const auto [n, u, v] = geom.orthonormal_basis(wi);
        return normal_dist(wh,u,v,n)*glm::dot(wh,n)/(4_f*glm::dot(wo, wh)*glm::dot(wo, n));
    }

    virtual Vec3 eval(const PointGeometry& geom, int, Vec3 wi, Vec3 wo) const override {
        if (geom.opposite(wi, wo)) {
            return {};
        }
        const auto wh = glm::normalize(wi + wo);
        const auto [n, u, v] = geom.orthonormal_basis(wi);
        const auto Fr = Ks_+(1_f-Ks_)*std::pow(1_f-dot(wo, wh),5_f);
        return Ks_*Fr*(normal_dist(wh,u,v,n)*shadowG(wi,wo,u,v,n)/(4_f*dot(wi,n)*dot(wo,n)));
    }

private:
    // Normal distribution of anisotropic GGX
    Float normal_dist(Vec3 wh, Vec3 u, Vec3 v, Vec3 n) const {
        return 1_f / (Pi*ax_*ay_*math::sq(math::sq(glm::dot(wh, u)/ax_) +
            math::sq(glm::dot(wh, v)/ay_) + math::sq(glm::dot(wh, n))));
    }

    // Smith's G term correspoinding to the anisotropic GGX
    Float shadowG(Vec3 wi, Vec3 wo, Vec3 u, Vec3 v, Vec3 n) const {
        const auto G1 = [&](Vec3 w) {
            const auto c = glm::dot(w, n);
            const auto s = std::max(Eps, math::safe_sqrt(1_f - c * c));
            const auto cp = glm::dot(w, u) / s;
            const auto cs = glm::dot(w, v) / s;
            const auto a2 = math::sq(cp * ax_) + math::sq(cs * ay_);
            return c == 0_f ? 0_f : 2_f / (1_f + math::safe_sqrt(1_f + a2 * math::sq(s / c)));
        };
        return G1(wi) * G1(wo);
    }
};

LM_COMP_REG_IMPL(Material_Glossy, "material::glossy");

LM_NAMESPACE_END(LM_NAMESPACE)
