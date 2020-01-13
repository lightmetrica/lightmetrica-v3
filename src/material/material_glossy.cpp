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

private:
    // Normal distribution of anisotropic GGX
    Float normal_dist(Vec3 wh, Vec3 u, Vec3 v, Vec3 n) const {
        return 1_f / (Pi*ax_*ay_*math::sq(math::sq(glm::dot(wh, u) / ax_) +
            math::sq(glm::dot(wh, v) / ay_) + math::sq(glm::dot(wh, n))));
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

public:
    virtual bool is_specular(const PointGeometry&, int) const override {
        return false;
    }

    virtual std::optional<MaterialDirectionSample> sample_direction(Rng& rng, const PointGeometry& geom, Vec3 wi) const override {
        const auto [n, u, v] = geom.orthonormal_basis_twosided(wi);
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
        const auto [n, u, v] = geom.orthonormal_basis_twosided(wi);
        return normal_dist(wh,u,v,n)*glm::dot(wh,n)/(4_f*glm::dot(wo, wh)*glm::dot(wo, n));
    }

    virtual Vec3 eval(const PointGeometry& geom, int, Vec3 wi, Vec3 wo) const override {
        if (geom.opposite(wi, wo)) {
            return {};
        }
        const auto wh = glm::normalize(wi + wo);
        const auto [n, u, v] = geom.orthonormal_basis_twosided(wi);
        const auto Fr = Ks_+(1_f-Ks_)*std::pow(1_f-dot(wo, wh),5_f);
        return Ks_*Fr*(normal_dist(wh,u,v,n)*shadowG(wi,wo,u,v,n)/(4_f*dot(wi,n)*dot(wo,n)));
    }
};

//LM_COMP_REG_IMPL(Material_Glossy, "material::glossy");

// ------------------------------------------------------------------------------------------------

// Taken from lmv2 for debugging
class Material_Glossy2 final : public Material {
private:
    Vec3 R_;
    Vec3 eta_;
    Vec3 k_;
    Float roughness_;

public:
    virtual void construct(const Json& prop) override {
        const auto Ks = json::value<Vec3>(prop, "Ks");
        const auto ax = json::value<Float>(prop, "ax");
        const auto ay = json::value<Float>(prop, "ay");

        R_ = Ks;
        eta_ = Vec3(0.140000_f, 0.129000_f, 0.158500_f);
        k_ = Vec3(4.586250_f, 3.348125_f, 2.329375_f);
        roughness_ = .1_f;
    }

private:
    Vec3 sample_GGX(Rng& rng) const
    {
        // Input u \in [0,1]^2
        const auto to_open_open = [](Float u) -> Float {
            return (1_f - 2_f * Eps) * u + Eps;
        };
        const auto to_open_closed = [](Float u) -> Float {
            return (1_f - Eps) * u + Eps;
        };

        // u0 \in (0,1]
        const auto u0 = to_open_closed(rng.u());
        // u1 \in (0,1)
        const auto u1 = to_open_open(rng.u());

        // Robust way of computation
        const auto cos_theta = [&]() -> Float {
            const auto v1 = math::safe_sqrt(1_f - u0);
            const auto v2 = math::safe_sqrt(1_f - (1_f - roughness_ * roughness_) * u0);
            return v1 / v2;
        }();
        const auto sin_theta = [&]() -> Float {
            const auto v1 = math::safe_sqrt(u0);
            const auto v2 = math::safe_sqrt(1_f - (1_f - roughness_ * roughness_) * u0);
            return roughness_ * (v1 / v2);
        }();
        const auto phi = Pi * (2_f * u1 - 1_f);
        return Vec3(
            sin_theta * glm::cos(phi),
            sin_theta * glm::sin(phi),
            cos_theta);
    }

    Float evaluate_GGX(const Vec3& H) const {
        const auto cosH = math::local_cos(H);
        const auto tanH = math::local_tan(H);
        if (cosH <= 0_f) return 0_f;
        const Float t1 = roughness_ * roughness_;
        const Float t2 = [&]() {
            const Float t = roughness_ * roughness_ + tanH * tanH;
            return Pi * cosH * cosH * cosH * cosH * t * t;
        }();
        return t1 / t2;
    }

public:
    virtual bool is_specular(const PointGeometry&, int) const override {
        return false;
    }

    virtual std::optional<MaterialDirectionSample> sample_direction(Rng& rng, const PointGeometry& geom, Vec3 wi) const override {
        const auto local_wi = geom.to_local * wi;
        if (math::local_cos(local_wi) <= 0_f) {
            return {};
        }

        const auto H = sample_GGX(rng);
        const auto local_wo = -local_wi - 2_f * glm::dot(-local_wi, H) * H;
        if (math::local_cos(local_wo) <= 0_f) {
            return {};
        }

        const auto wo = geom.to_world * local_wo;
        const auto f = eval(geom, {}, wi, wo);
        const auto p = pdf_direction(geom, {}, wi, wo);
        return MaterialDirectionSample{
            wo,
            SurfaceComp::DontCare,
            f / p
        };
    }

    virtual std::optional<Vec3> sample_direction_given_comp(Rng& rng, const PointGeometry& geom, int, Vec3 wi) const override {
        return {};
    }

    virtual std::optional<Vec3> reflectance(const PointGeometry&, int) const override {
        return R_;
    }

    virtual Float pdf_direction(const PointGeometry& geom, int, Vec3 wi, Vec3 wo) const override {
        const auto local_wi = geom.to_local * wi;
        const auto local_wo = geom.to_local * wo;
        if (math::local_cos(local_wi) <= 0_f || math::local_cos(local_wo) <= 0_f) {
            return 0_f;
        }

        const auto H = glm::normalize(local_wi + local_wo);
        const auto D = evaluate_GGX(H);
        return D * math::local_cos(H) / (4_f * glm::dot(local_wo, H)) / math::local_cos(local_wo);
    }

    virtual Vec3 eval(const PointGeometry& geom, int, Vec3 wi, Vec3 wo) const override {
        LM_TBA();
    }
};

LM_COMP_REG_IMPL(Material_Glossy2, "material::glossy");

LM_NAMESPACE_END(LM_NAMESPACE)
