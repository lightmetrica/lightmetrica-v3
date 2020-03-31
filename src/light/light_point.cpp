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

    virtual std::optional<RaySample> sample_ray(const RaySampleU& us, const Transform&) const override {
        const auto d = math::sample_uniform_sphere(us.ud);
        const auto geomL = PointGeometry::make_degenerated(position_);
        const auto p = math::pdf_uniform_sphere();
        const auto C = Le_ / p;
        return RaySample{
            geomL,
            d,
            C
        };
    }

    virtual Float pdf_ray(const PointGeometry&, Vec3, const Transform&) const override {
        return math::pdf_uniform_sphere();
    }

    // --------------------------------------------------------------------------------------------

    virtual std::optional<DirectionSample> sample_direction(const PointGeometry&, const DirectionSampleU& us) const override {
        const auto d = math::sample_uniform_sphere(us.ud);
        const auto C = Le_ / math::pdf_uniform_sphere();
        return DirectionSample{
            d,
            C
        };   
    }

    virtual Float pdf_direction(const PointGeometry&, Vec3) const override {
        return math::pdf_uniform_sphere();
    }

    // --------------------------------------------------------------------------------------------

    virtual std::optional<PositionSample> sample_position(const PositionSampleU&, const Transform&) const override {
        return PositionSample{
            PointGeometry::make_degenerated(position_),
            Vec3(1_f)
        };
    }

    virtual Float pdf_position(const PointGeometry&, const Transform&) const override {
        return 1_f;
    }

    // --------------------------------------------------------------------------------------------

    virtual std::optional<RaySample> sample_direct(const RaySampleU&, const PointGeometry& geom, const Transform&) const override {
        const auto wo = glm::normalize(geom.p - position_);
        const auto geomL = PointGeometry::make_degenerated(position_);
        const auto pL = pdf_direct(geom, geomL, {}, wo);
        if (pL == 0_f) {
            return {};
        }
        const auto C = Le_ / pL;
        return RaySample{
            geomL,
            wo,
            C
        };
    }

    virtual Float pdf_direct(const PointGeometry& geom, const PointGeometry& geomL, const Transform&, Vec3) const override {
        const auto G = surface::geometry_term(geom, geomL);
        return G == 0_f ? 0_f : 1_f / G;
    }

    // --------------------------------------------------------------------------------------------

    virtual bool is_infinite() const override {
        return false;
    }

    virtual bool is_connectable(const PointGeometry&) const override {
        return true;
    }

    virtual Vec3 eval(const PointGeometry&, Vec3) const override {
        return Le_;
    }
};

LM_COMP_REG_IMPL(Light_Point, "light::point");

LM_NAMESPACE_END(LM_NAMESPACE)
