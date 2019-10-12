/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/core.h>
#include <lm/light.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*
\rst
.. function:: light::directional

    Directional light.

    :param color Le: Luminance.
    :param vec3 direction: Direction of the light.
\endrst
*/
class Light_Directional final : public Light {
private:
    Vec3 Le_;           // Luminance
    Vec3 direction_;    // Direction of the light
    
public:
    LM_SERIALIZE_IMPL(ar) {
        ar(Le_, direction_);
    }

public:
    virtual void construct(const Json& prop) override {
        Le_ = json::value<Vec3>(prop, "Le");
        direction_ = glm::normalize(json::value<Vec3>(prop, "direction"));
    }

    virtual std::optional<LightRaySample> sample(Rng&, const PointGeometry& geom, const Transform&) const override {
        const auto geomL = PointGeometry::makeInfinite(direction_);
        const auto pL = pdf(geom, geomL, 0, {}, direction_);
        if (pL == 0_f) {
            return {};
        }
        return LightRaySample{
            geomL,
            direction_,
            0,
            Le_ / pL
        };
    }

    virtual Float pdf(const PointGeometry& geom, const PointGeometry& geomL, int, const Transform&, Vec3) const override {
        const auto d = -geomL.wo;
        return surface::convertSAToProjSA(1_f, geom, d);
    }

    virtual bool isSpecular(const PointGeometry&, int) const override {
        return true;
    }

    virtual bool isInfinite() const override {
        return false;
    }

    virtual Vec3 eval(const PointGeometry&, int, Vec3) const override {
        return Le_;
    }
};

LM_COMP_REG_IMPL(Light_Directional, "light::directional");

LM_NAMESPACE_END(LM_NAMESPACE)
