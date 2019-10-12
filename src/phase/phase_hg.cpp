/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/core.h>
#include <lm/phase.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Phase_HenyeyGreenstein final : public Phase {
private:
    Float g_;   // Asymmetry parameter in [-1,1]

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(g_);
    }

    virtual void construct(const Json& prop) override {
        g_ = json::value<Float>(prop, "g");
    }

public:
    virtual bool isSpecular(const PointGeometry&) const override {
        return false;
    }

    virtual std::optional<PhaseDirectionSample> sample(Rng& rng, const PointGeometry&, Vec3 wi) const override {
        const auto cosT = [&]() -> Float {
            if (std::abs(g_) < Eps) {
                return 1_f - 2_f*rng.u();
            }
            else {
                const auto sq = (1_f-g_*g_)/(1_f-g_+2*g_*rng.u());
                return (1_f+g_*g_-sq*sq)/(2_f*g_);
            }
        }();
        const auto sinT = math::safeSqrt(1_f-cosT*cosT);
        const auto phi = 2_f * Pi * rng.u();
        const auto sinP = std::sin(phi);
        const auto cosP = std::cos(phi);
        const auto localWo = Vec3(sinT*cosP, sinT*sinP, cosT);
        const auto [u, v] = math::orthonormalBasis(-wi);
        const auto wo = Mat3(u, v, -wi) * localWo;
        return PhaseDirectionSample{ wo, Vec3(1_f) };
    }

    virtual Float pdf(const PointGeometry&, Vec3 wi, Vec3 wo) const override {
        Float t = 1_f + g_*g_ + 2_f*g_*glm::dot(wi, wo);
        return (1_f-g_*g_) / (t*std::sqrt(t)) / (Pi*4_f);
    }

    virtual Vec3 eval(const PointGeometry& geom, Vec3 wi, Vec3 wo) const override {
        return Vec3(pdf(geom, wi, wo));
    }
};

LM_COMP_REG_IMPL(Phase_HenyeyGreenstein, "phase::hg");

LM_NAMESPACE_END(LM_NAMESPACE)
