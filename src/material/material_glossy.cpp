/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/material.h>
#include <lm/json.h>
#include <lm/serial.h>
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
    virtual bool construct(const Json& prop) override {
        Ks_ = prop["Ks"];
        ax_ = prop["ax"];
        ay_ = prop["ay"];
        return true;
    }

public:
    virtual bool isSpecular(const PointGeometry&, int) const override {
        return false;
    }

    virtual std::optional<MaterialDirectionSample> sample(Rng& rng, const PointGeometry& geom, Vec3 wi) const override {
        const auto [n, u, v] = geom.orthonormalBasis(wi);
        const auto u1 = rng.u() * 2_f * Pi;
        const auto u2 = rng.u();
        const auto wh = glm::normalize(math::safeSqrt(u2/(1_f-u2))*(ax_*glm::cos(u1)*u+ay_*glm::sin(u1)*v)+n);
        const auto wo = math::reflection(wi, wh);
        if (geom.opposite(wi, wo)) {
            return {};
        }
        return MaterialDirectionSample{
            wo,
            SurfaceComp::DontCare,
            eval(geom, {}, wi, wo) / pdf(geom, {}, wi, wo)
        };
    }

    virtual std::optional<Vec3> reflectance(const PointGeometry&, int) const override {
        return Ks_;
    }

    virtual Float pdf(const PointGeometry& geom, int, Vec3 wi, Vec3 wo) const override {
        if (geom.opposite(wi, wo)) {
            return 0_f;
        }
        const auto wh = glm::normalize(wi + wo);
        const auto [n, u, v] = geom.orthonormalBasis(wi);
        return normalDist(wh,u,v,n)*glm::dot(wh,n)/(4_f*glm::dot(wo, wh)*glm::dot(wo, n));
    }

    virtual Float pdfComp(const PointGeometry&, int, Vec3) const override {
        return 1_f;
    }

    virtual Vec3 eval(const PointGeometry& geom, int, Vec3 wi, Vec3 wo) const override {
        if (geom.opposite(wi, wo)) {
            return {};
        }
        const auto wh = glm::normalize(wi + wo);
        const auto [n, u, v] = geom.orthonormalBasis(wi);
        const auto Fr = Ks_+(1_f-Ks_)*std::pow(1_f-dot(wo, wh),5_f);
        return Ks_*Fr*(normalDist(wh,u,v,n)*shadowG(wi,wo,u,v,n)/(4_f*dot(wi,n)*dot(wo,n)));
    }

private:
    // Normal distribution of anisotropic GGX
    Float normalDist(Vec3 wh, Vec3 u, Vec3 v, Vec3 n) const {
        return 1_f / (Pi*ax_*ay_*math::sq(math::sq(glm::dot(wh, u)/ax_) +
            math::sq(glm::dot(wh, v)/ay_) + math::sq(glm::dot(wh, n))));
    }

    // Smith's G term correspoinding to the anisotropic GGX
    Float shadowG(Vec3 wi, Vec3 wo, Vec3 u, Vec3 v, Vec3 n) const {
        const auto G1 = [&](Vec3 w) {
            const auto c = glm::dot(w, n);
            const auto s = std::max(Eps, math::safeSqrt(1_f - c * c));
            const auto cp = glm::dot(w, u) / s;
            const auto cs = glm::dot(w, v) / s;
            const auto a2 = math::sq(cp * ax_) + math::sq(cs * ay_);
            return c == 0_f ? 0_f : 2_f / (1_f + math::safeSqrt(1_f + a2 * math::sq(s / c)));
        };
        return G1(wi) * G1(wo);
    }
};

LM_COMP_REG_IMPL(Material_Glossy, "material::glossy");

LM_NAMESPACE_END(LM_NAMESPACE)
