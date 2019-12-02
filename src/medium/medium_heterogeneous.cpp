/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/core.h>
#include <lm/medium.h>
#include <lm/surface.h>
#include <lm/phase.h>
#include <lm/volume.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Medium_Heterogeneous final : public Medium {
private:
    const Volume* volume_density_;  // Density volume. density := \mu_t = \mu_a + \mu_s
    const Volume* volme_albedo_;	// Albedo volume. albedo := \mu_s / \mu_t
    const Phase* phase_;            // Underlying phase function.

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(volume_density_, volme_albedo_, phase_);
    }

public:
    virtual void construct(const Json& prop) override {
        volume_density_ = json::comp_ref<Volume>(prop, "volume_density");
        volme_albedo_ = json::comp_ref<Volume>(prop, "volume_albedo");
        phase_ = json::comp_ref<Phase>(prop, "phase");
    }

    virtual std::optional<MediumDistanceSample> sample_distance(Rng& rng, Ray ray, Float tmin, Float tmax) const override {
        // Compute overlapping range between the ray and the volume
        if (!volume_density_->bound().isect_range(ray, tmin, tmax)) {
            // No intersection with the volume, use surface interaction
            return {};
        }
        
        // Sample distance by delta tracking
        Float t = tmin;
        const auto inv_max_density = 1_f / volume_density_->max_scalar();
        while (true) {
            // Sample a distance from the 'homogenized' volume
            t -= glm::log(1_f-rng.u()) * inv_max_density;
            if (t >= tmax) {
                // Hit with boundary, use surface interaction
                return {};
            }

            // Density at the sampled point
            const auto p = ray.o + ray.d*t;
            const auto density = volume_density_->eval_scalar(p);

            // Determine scattering collision or null collision
            // Continue tracking if null collusion is seleced
            if (density * inv_max_density > rng.u()) {
                // Scattering collision
                const auto albedo = volme_albedo_->eval_color(p);
                return MediumDistanceSample{
                    ray.o + ray.d*t,
                    albedo,     // T_{\bar{\mu}}(t) / p_{\bar{\mu}}(t) * \mu_s(t)
                                // = 1/\mu_t(t) * \mu_s(t) = albedo(t)
                    true
                };
            }
        }

        LM_UNREACHABLE_RETURN();
    }
    
    virtual Vec3 eval_transmittance(Rng& rng, Ray ray, Float tmin, Float tmax) const override {
        // Compute overlapping range
        if (!volume_density_->bound().isect_range(ray, tmin, tmax)) {
            // No intersection with the volume, no attenuation
            return Vec3(1_f);
        }

        // Perform ratio tracking [Novak et al. 2014]
        Float Tr = 1_f;
        Float t = tmin;
        const auto inv_max_density = 1_f / volume_density_->max_scalar();
        while (true) {
            t -= glm::log(1_f - rng.u()) * inv_max_density;
            if (t >= tmax) {
                break;
            }
            const auto p = ray.o + ray.d*t;
            const auto density = volume_density_->eval_scalar(p);
            Tr *= 1_f - density * inv_max_density;
        }

        return Vec3(Tr);
    }

    virtual bool is_emitter() const override {
        return false;
    }

    virtual const Phase* phase() const override {
        return phase_;
    }
};

LM_COMP_REG_IMPL(Medium_Heterogeneous, "medium::heterogeneous");

LM_NAMESPACE_END(LM_NAMESPACE)
