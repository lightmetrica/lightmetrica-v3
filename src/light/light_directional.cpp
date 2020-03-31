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
    SphereBound sphere_bound_;
    Vec3 Le_;           // Luminance
    Vec3 direction_;    // Direction of the light
    
public:
    LM_SERIALIZE_IMPL(ar) {
        ar(sphere_bound_, Le_, direction_);
    }

public:
    virtual void construct(const Json& prop) override {
        Le_ = json::value<Vec3>(prop, "Le");
        direction_ = glm::normalize(json::value<Vec3>(prop, "direction"));
    }

    // --------------------------------------------------------------------------------------------

    virtual void set_scene_bound(const Bound& bound) {
        // Compute the bounding sphere of the scene.
        // Although it is inefficient, currently we just use a convervative bound of AABB.
        sphere_bound_.center = (bound.max + bound.min) * .5_f;
        sphere_bound_.radius = glm::length(bound.max - sphere_bound_.center) * 1.01_f;
    }

    // --------------------------------------------------------------------------------------------

    virtual std::optional<RaySample> sample_ray(const RaySampleU& us, const Transform&) const override {
        // Sample position on the virtual disk
        const auto d = direction_;
        const auto p_local = math::sample_uniform_disk(us.up) * sphere_bound_.radius;
        const auto [u, v] = math::orthonormal_basis(d);
        const auto p_world = sphere_bound_.center + sphere_bound_.radius * (-d) + (u * p_local.x + v * p_local.y);
        const auto geomL = PointGeometry::make_infinite(d, p_world);

        // Contribution
        const auto Le = eval(geomL, d, false);
        const auto p = pdf_ray(geomL, d, {}, false);
        const auto C = Le / p;

        return RaySample{
            geomL,
            d,
            C
        };
    }

    virtual Float pdf_ray(const PointGeometry&, Vec3, const Transform&, bool eval_delta) const override {
        const auto pA = 1_f / (Pi * sphere_bound_.radius * sphere_bound_.radius);
        return eval_delta ? 1_f : pA;
    }

    // --------------------------------------------------------------------------------------------

    virtual std::optional<DirectionSample> sample_direction(const PointGeometry&, const DirectionSampleU&) const override {
        LM_THROW_EXCEPTION_DEFAULT(Error::Unsupported);
    }

    virtual Float pdf_direction(const PointGeometry&, Vec3) const override {
        LM_THROW_EXCEPTION_DEFAULT(Error::Unsupported);
    }

    // --------------------------------------------------------------------------------------------

    virtual std::optional<PositionSample> sample_position(const PositionSampleU&, const Transform&) const override {
        LM_THROW_EXCEPTION_DEFAULT(Error::Unsupported);
    }

    virtual Float pdf_position(const PointGeometry&, const Transform&) const override {
        LM_THROW_EXCEPTION_DEFAULT(Error::Unsupported);
    }

    // --------------------------------------------------------------------------------------------

    virtual std::optional<RaySample> sample_direct(const RaySampleU&, const PointGeometry& geom, const Transform&) const override {
        const auto geomL = PointGeometry::make_infinite(direction_);
        const auto pL = pdf_direct(geom, geomL, {}, direction_, false);
        if (pL == 0_f) {
            return {};
        }
        return RaySample{
            geomL,
            direction_,
            Le_ / pL
        };
    }

    virtual Float pdf_direct(const PointGeometry& geom, const PointGeometry& geomL, const Transform&, Vec3, bool eval_delta) const override {
        const auto d = -geomL.wo;
        return eval_delta ? 0_f : surface::convert_pdf_SA_to_projSA(1_f, geom, d);
    }

    // --------------------------------------------------------------------------------------------

    virtual bool is_specular() const override {
        return true;
    }

    virtual bool is_infinite() const override {
        return true;
    }

    virtual bool is_connectable(const PointGeometry&) const override {
        return false;
    }

    virtual Vec3 eval(const PointGeometry&, Vec3, bool eval_delta) const override {
        return eval_delta ? Vec3(0_f) : Le_;
    }
};

LM_COMP_REG_IMPL(Light_Directional, "light::directional");

LM_NAMESPACE_END(LM_NAMESPACE)
