/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/core.h>
#include <lm/light.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
\rst
.. function:: light::point

    Point light.

    :param color Le: Luminance.
    :param vec3 position: Position of the light.
\endrst
*/
class Light_Point final : public Light {
private:
    Vec3 Le_;           // Luminance
    Vec3 position_;     // Position of the light

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(Le_, position_);
    }

public:
    virtual void construct(const Json& prop) override {
        Le_ = json::value<Vec3>(prop, "Le");
        position_ = json::value<Vec3>(prop, "position");
    }

    virtual std::optional<LightRaySample> sample(Rng&, const PointGeometry& geom, const Transform&) const override {
        const auto wo = glm::normalize(geom.p - position_);
        const auto geomL = PointGeometry::make_degenerated(position_);
        const auto pL = pdf(geom, geomL, 0, {}, wo);
        if (pL == 0_f) {
            return {};
        }
        return LightRaySample{
            geomL,
            wo,
            0,
            Le_ / pL
        };
    }

    virtual Float pdf(const PointGeometry& geom, const PointGeometry& geomL, int, const Transform&, Vec3) const override {
        const auto G = surface::geometry_term(geom, geomL);
        return G == 0_f ? 0_f : 1_f / G;
    }

    virtual bool is_specular(const PointGeometry&, int) const override {
        return false;
    }

    virtual bool is_infinite() const override {
        return false;
    }

    virtual Vec3 eval(const PointGeometry&, int, Vec3) const override {
        return Le_;
    }
};

LM_COMP_REG_IMPL(Light_Point, "light::point");

LM_NAMESPACE_END(LM_NAMESPACE)
