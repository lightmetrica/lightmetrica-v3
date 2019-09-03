/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/medium.h>
#include <lm/json.h>
#include <lm/serial.h>
#include <lm/surface.h>
#include <lm/phase.h>
#include <lm/volume.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Medium_Heterogeneous final : public Medium {
private:
	const Volume* volumeDensity_;	// Density volume.
	const Volume* volmeAlbedo_;		// Albedo volume.
    const Phase* phase_;			// Underlying phase function.

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(volumeDensity_, volmeAlbedo_, phase_);
    }

public:
    virtual bool construct(const Json& prop) override {
		volumeDensity_ = json::compRef<Volume>(prop, "volume_density");
		volmeAlbedo_ = json::compRef<Volume>(prop, "volume_albedo");
        phase_ = json::compRef<Phase>(prop, "phase");
        return true;
    }

    virtual std::optional<MediumDistanceSample> sampleDistance(Rng& rng, const PointGeometry& geom, Vec3 wo, Float distToSurf) const override {
        // Compute overlapping range between the ray and the volume
        Float tmin = 0_f;
        Float tmax = distToSurf;
        if (!volumeDensity_->bound().isectRange(Ray{ geom.p, wo }, tmin, tmax)) {
            // No intersection with the volume, use surface interaction
            return {};
        }
        
        // Sample distance by delta tracking
        Float t = tmin;
        const auto invMaxDensity = 1_f / volumeDensity_->maxScalar();
        while (true) {
            // Sample a distance from the 'homogenized' volume
            t -= glm::log(1_f-rng.u()) * invMaxDensity;
            if (t >= tmax) {
                // Hit with boundary, use surface interaction
                return {};
            }

            // Density at the sampled point
            const auto p = geom.p + wo * t;
            const auto density = volumeDensity_->evalScalar(p);

            // Determine scattering collision or null collision
            if (density * invMaxDensity > rng.u()) {
                // Scattering collision
                const auto albedo = volmeAlbedo_->evalColor(p);
                return MediumDistanceSample{
                    geom.p + wo*t,
                    albedo * density,
                    true
                };
            }
        }

        LM_UNREACHABLE_RETURN();
    }
    
    virtual std::optional<Vec3> evalTransmittance(Rng& rng, const PointGeometry& geom1, const PointGeometry& geom2) const override {
        // Direction from p1 to p2
        const auto wo = glm::normalize(geom2.p - geom1.p);

        // Compute overlapping range
        Float tmin = 0_f;
        Float tmax = glm::distance(geom1.p, geom2.p);
        if (!volumeDensity_->bound().isectRange(Ray{ geom1.p, wo }, tmin, tmax)) {
            // No intersection with the volume, no attenuation
            return Vec3(1_f);
        }

        // Perform ratio tracking [Novak et al. 2014]
        Float Tr = 1_f;
        Float t = tmin;
        const auto invMaxDensity = 1_f / volumeDensity_->maxScalar();
        while (true) {
            t -= glm::log(1_f - rng.u()) * invMaxDensity;
            if (t >= tmax) {
                break;
            }
            const auto p = geom1.p + wo * t;
            const auto density = volumeDensity_->evalScalar(p);
            Tr *= 1_f - density * invMaxDensity;
        }

        return Vec3(Tr);
    }

    virtual bool isEmitter() const override {
        return false;
    }

    virtual const Phase* phase() const override {
        return phase_;
    }
};

LM_COMP_REG_IMPL(Medium_Heterogeneous, "medium::heterogeneous");

LM_NAMESPACE_END(LM_NAMESPACE)
