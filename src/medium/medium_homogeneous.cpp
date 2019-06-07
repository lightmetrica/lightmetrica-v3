/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/medium.h>
#include <lm/json.h>
#include <lm/serial.h>
#include <lm/surface.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// Assume the medium is non-emissive
class Medium_Homogeneous final : public Medium {
private:
    Float muA_;     // Absorption coefficient
    Float muS_;     // Scattering coefficient
    Float muT_;     // Extinction coefficient

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(muA_, muS_, muT_);
    }

public:
    virtual bool construct(const Json& prop) override {
        muA_ = json::value<Float>(prop, "muA");
        muS_ = json::value<Float>(prop, "muS");
        muT_ = muA_ + muS_;
    }

    /*
        Memo.
        - Transmittance T(t) = exp[ -\int_0^t \mu_t(x+s\omega) ds ] = exp[-\mu_t t].
        - PDF p(t) = \mu_t exp[-\mu_t t].
        - CDF F(t) = 1 - T(t).
        - Sampled t ~ p(t). t = F^-1(U) = -ln(1-U)/\mu_t.
        - Prob. of surface interaction P[t>s] = 1-F(s) = T(s).
        - Weight for medium interaction. \mu_s T(t)/p(t) = \mu_s/\mu_t
        - Weight for surface interaction. T(s)/P[t>s] = 1
    */
    virtual std::optional<MediumDistanceSample> sampleDistance(Rng& rng, const PointGeometry& geom, Vec3 wo, Float distToSurf) const override {
        // Sample a distance
        const auto t = -std::log(1_f-rng.u()) / muT_;
        
        if (t < distToSurf) {
            // Medium interaction
            return MediumDistanceSample{
                geom.p + wo*t,
                Vec3(muS_ / muT_)
            };
        }
        else {
            // Surface interaction
            return MediumDistanceSample{
                geom.p + wo*distToSurf,
                Vec3(1_f)
            };
        }

        LM_UNREACHABLE_RETURN();
    }
};

LM_COMP_REG_IMPL(Medium_Homogeneous, "material::diffuse");

LM_NAMESPACE_END(LM_NAMESPACE)
