/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/core.h>
#include <lm/medium.h>
#include <lm/phase.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// Assume the medium is non-emissive
class Medium_Homogeneous final : public Medium {
private:
    Float density_;         // Density of volume := extinction coefficient \mu_t
    Vec3 albedo_;           // Albedo of volume := \mu_s / \mu_t
    Vec3 muA_;              // Absorption coefficient.
    Vec3 muS_;              // Scattering coefficient.
    const Phase* phase_;    // Underlying phase function.
    Bound bound_;

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(density_, muA_, muS_, phase_, bound_);
    }

public:
    virtual void construct(const Json& prop) override {
        density_ = json::value<Float>(prop, "density");
        albedo_ = json::value<Vec3>(prop, "albedo");
        muS_ = albedo_ * density_;
        muA_ = density_ - muS_;
        phase_ = json::comp_ref<Phase>(prop, "phase");
        bound_.min = json::value<Vec3>(prop, "bound_min", Vec3(-Inf));
        bound_.max = json::value<Vec3>(prop, "bound_max", Vec3(Inf));
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
    virtual std::optional<MediumDistanceSample> sample_distance(Rng& rng, Ray ray, Float tmin, Float tmax) const override {
        // Compute overlapping range between volume and bound
        if (!bound_.isect_range(ray, tmin, tmax)) {
            // No intersection with volume, use surface interaction
            return {};
        }
        
        // Sample a distance
        const auto t = -std::log(1_f-rng.u()) / density_;
        
        if (t < tmax - tmin) {
            // Medium interaction
            return MediumDistanceSample{
                ray.o + ray.d*(tmin+t),
                albedo_,    // \mu_s / \mu_t
                true
            };
        }
        else {
            // Surface interaction
            return MediumDistanceSample{
                ray.o + ray.d*tmax,
                Vec3(1_f),
                false
            };
        }
    }

    virtual Vec3 eval_transmittance(Rng&, Ray ray, Float tmin, Float tmax) const override {
        // Compute overlapping range
        if (!bound_.isect_range(ray, tmin, tmax)) {
            // No intersection with the volume, no attenuation
            return Vec3(1_f);
        }
        return Vec3(std::exp(-density_ * (tmax - tmin)));
    }

    virtual bool is_emitter() const override {
        return false;
    }

    virtual const Phase* phase() const override {
        return phase_;
    }
};

LM_COMP_REG_IMPL(Medium_Homogeneous, "medium::homogeneous");

LM_NAMESPACE_END(LM_NAMESPACE)
