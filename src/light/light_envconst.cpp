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
.. function:: light::envconst

    Constant environment light.

    :param color Le: Luminance.
\endrst
*/
class Light_EnvConst final : public Light {
private:
    Vec3 Le_;
    struct SphereBound {
        Vec3 center;
        Float radius;
    };
    SphereBound sphere_bound_;
    
public:
    LM_SERIALIZE_IMPL(ar) {
        ar(Le_);
    }
    
public:
    virtual void construct(const Json& prop) override {
        Le_ = json::value<Vec3>(prop, "Le");
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
        // Sample direction
        const auto d = math::sample_uniform_sphere(us.ud);
        
        // Sample a position on the disk perpendicular to the sampled direction,
        // where the radius of the disk is the radius of bounding sphere of the scene.
        const auto p_local = math::sample_uniform_disk(us.up) * sphere_bound_.radius;
        const auto [u, v] = math::orthonormal_basis(d);
        const auto p_world = sphere_bound_.center + sphere_bound_.radius * (-d) + (u*p_local.x + v*p_local.y);
        const auto geomL = PointGeometry::make_infinite(d, p_world);

        // Evaluate contribution
        const auto Le = eval(geomL, d);
        const auto p = pdf_ray(geomL, d, {});
        const auto C = Le / p;

        return RaySample{
            geomL,
            d,
            C
        };
    }

    virtual Float pdf_ray(const PointGeometry& geom, Vec3 wo, const Transform&) const override {
        const auto pD = math::pdf_uniform_sphere();
        const auto pA = 1_f / (Pi * sphere_bound_.radius * sphere_bound_.radius);
        return pD * pA;
    }

    // --------------------------------------------------------------------------------------------

    // Direction and position sampling is disabled
    // since they are only sampled from the joint distribution.
    virtual std::optional<DirectionSample> sample_direction(const PointGeometry& geom, const DirectionSampleU& u) const override {
        LM_THROW_EXCEPTION_DEFAULT(Error::Unsupported);
    }

    virtual Float pdf_direction(const PointGeometry& geom, Vec3 wo) const override {
        LM_THROW_EXCEPTION_DEFAULT(Error::Unsupported);
    }

    // --------------------------------------------------------------------------------------------

    virtual std::optional<PositionSample> sample_position(const PositionSampleU& u, const Transform& transform) const override {
        LM_THROW_EXCEPTION_DEFAULT(Error::Unsupported);
    }

    virtual Float pdf_position(const PointGeometry& geom, const Transform& transform) const override {
        LM_THROW_EXCEPTION_DEFAULT(Error::Unsupported);
    }

    // --------------------------------------------------------------------------------------------

    virtual std::optional<RaySample> sample_direct(const RaySampleU& u, const PointGeometry& geom, const Transform& transform) const override {
        
    }

    virtual Float pdf_direct(const PointGeometry& geom, const PointGeometry& geomL, const Transform& transform, Vec3 wo) const override {
        
    }

    // --------------------------------------------------------------------------------------------

    virtual Vec3 eval(const PointGeometry& geom, Vec3 wo) const override {
        
    }

#if 0
    virtual std::optional<LightRaySample> sample_direct(Rng& rng, const PointGeometry& geom, const Transform&) const override {
        const auto wo = math::sample_uniform_sphere(rng);
        const auto geomL = PointGeometry::make_infinite(wo);
        const auto pL = pdf_direct(geom, geomL, 0, {}, wo);
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

    virtual Float pdf_direct(const PointGeometry& geom, const PointGeometry& geomL, int, const Transform&, Vec3) const override {
        const auto d = -geomL.wo;
        return surface::convert_SA_to_projSA(math::pdf_uniform_sphere(), geom, d);
    }

    virtual bool is_specular(const PointGeometry&, int) const override {
        return false;
    }

    virtual bool is_infinite() const override {
        return true;
    }

    virtual Vec3 eval(const PointGeometry&, int, Vec3) const override {
        return Le_;
    }
#endif
};

LM_COMP_REG_IMPL(Light_EnvConst, "light::envconst");

LM_NAMESPACE_END(LM_NAMESPACE)
