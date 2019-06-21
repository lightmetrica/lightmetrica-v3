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

LM_NAMESPACE_BEGIN(LM_NAMESPACE)


// Memo:
// Currently we forget about muA. Assume muA=0
// density = muT = muS
namespace volume {
    Float maxDensity() {
        return 1_f;
    }

    Float evalDensity(Vec3 p, const Bound& b) {
        const auto Delta = .2_f;
        const auto t = (p - b.mi) / (b.ma - b.mi);
        const auto x = int(t.x / Delta);
        const auto y = int(t.y / Delta);
        //const auto z = int(t.z / Delta);
        return (x + y) % 2 == 0 ? 1_f : 0_f;
    }

    Vec3 evalAlbedo(Vec3 , const Bound& ) {
        //const auto t = (p - b.mi) / (b.ma - b.mi);
        return Vec3(.5_f);
    }
}

class Medium_Heterogeneous final : public Medium {
private:
    Bound bound_;           // Bound of volume
    const Phase* phase_;    // Underlying phase function.

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(phase_);
    }

public:
    virtual bool construct(const Json& prop) override {
        bound_.mi = json::value<Vec3>(prop, "bound_min");
        bound_.ma = json::value<Vec3>(prop, "bound_max");
        phase_ = comp::get<Phase>(prop["phase"]);
        if (!phase_) {
            return false;
        }
        return true;
    }

    virtual std::optional<MediumDistanceSample> sampleDistance(Rng& rng, const PointGeometry& geom, Vec3 wo, Float distToSurf) const override {
        // Compute overlapping range between the ray and the volume
        Float tmin = 0_f;
        Float tmax = distToSurf;
        if (!bound_.isectRange(Ray{ geom.p, wo }, tmin, tmax)) {
            // No intersection with the volume, use surface interaction
            return {};
        }
        
        // Sample distance by delta tracking
        Float t = tmin;
        const auto invMaxDensity = 1_f / volume::maxDensity();
        while (true) {
            // Sample a distance from the 'homogenized' volume
            t -= glm::log(1_f-rng.u()) * invMaxDensity;
            if (t >= tmax) {
                // Hit with boundary, use surface interaction
                return {};
            }

            // Density at the sampled point
            const auto p = geom.p + wo * t;
            const auto density = volume::evalDensity(p, bound_);

            // Determine scattering collision or null collision
            if (density * invMaxDensity > rng.u()) {
                // Scattering collision
                const auto albedo = volume::evalAlbedo(p, bound_);
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
        if (!bound_.isectRange(Ray{ geom1.p, wo }, tmin, tmax)) {
            // No intersection with the volume, no attenuation
            return Vec3(1_f);
        }

        // Perform ratio tracking [Novak et al. 2014]
        Float Tr = 1_f;
        Float t = tmin;
        const auto invMaxDensity = 1_f / volume::maxDensity();
        while (true) {
            t -= glm::log(1_f - rng.u()) * invMaxDensity;
            if (t >= tmax) {
                break;
            }
            const auto p = geom1.p + wo * t;
            const auto density = volume::evalDensity(p, bound_);
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
