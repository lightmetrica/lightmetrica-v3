/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/core.h>
#include <lm/light.h>
#include <lm/texture.h>

#define LIGHT_ENV_DEBUG_USE_CONST_TEXTURE 0

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*
\rst
.. function:: light::env

    Environment light.

    :param str envmap_path: Path to environment map.
    :param float rot: Rotation angle of the environment map around up vector in degrees.
                      Default value: 0.
\endrst
*/
class Light_Env final : public Light {
private:
    SphereBound sphere_bound_;
    Component::Ptr<Texture> envmap_;    // Environment map
    Float rot_;                         // Rotation of the environment map around (0,1,0)
    Dist2 dist_;                        // For sampling directions

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(sphere_bound_, envmap_, rot_, dist_);
    }

    virtual void foreach_underlying(const ComponentVisitor& visitor) override {
        comp::visit(visitor, envmap_);
    }

    virtual Component* underlying(const std::string& name) const override {
        if (name == "envmap") {
            return envmap_.get();
        }
        return nullptr;
    }

public:
    virtual void construct(const Json& prop) override {
        #if LIGHT_ENV_DEBUG_USE_CONST_TEXTURE
        envmap_ = comp::create<Texture>("texture::constant", make_loc("envmap"), {
            {"color", Vec3(1_f)}
        });
        #else
        envmap_ = comp::create<Texture>("texture::bitmap", make_loc("envmap"), prop);
        if (!envmap_) {
            LM_THROW_EXCEPTION_DEFAULT(Error::InvalidArgument);
        }
        #endif
        rot_ = glm::radians(json::value(prop, "rot", 0_f));
        const auto [w, h] = envmap_->size();
        std::vector<Float> ls(w * h);
        for (int i = 0; i < w*h; i++) {
            const int x = i % w;
            const int y = i / w;
            const auto v = envmap_->eval_by_pixel_coords(x, y);
            ls[i] = glm::compMax(v) * std::sin(Pi * (Float(i) / w + .5_f) / h);
        }
        dist_.init(ls, w, h);
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

    virtual Float pdf_ray(const PointGeometry&, Vec3, const Transform&) const override {
        const auto pD = math::pdf_uniform_sphere();
        const auto pA = 1_f / (Pi * sphere_bound_.radius * sphere_bound_.radius);
        return pD * pA;
    }

    // --------------------------------------------------------------------------------------------

    // Direction and position sampling is disabled
    // since they are only sampled from the joint distribution.
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

    virtual std::optional<RaySample> sample_direct(const RaySampleU& u_, const PointGeometry& geom, const Transform&) const override {
        const auto u = dist_.sample({ u_.ud, u_.up });
        const auto t  = Pi * u[1];
        const auto st = sin(t);
        const auto p  = 2 * Pi * u[0] + rot_;
        const auto wo = -Vec3(st * sin(p), cos(t), st * cos(p));
        const auto geomL = PointGeometry::make_infinite(wo);
        const auto pL = pdf_direct(geom, geomL, {}, wo);
        if (pL == 0_f) {
            return {};
        }
        const auto Le = eval(geomL, wo);
        const auto C = Le / pL;
        return RaySample{
            geomL,
            wo,
            C
        };
    }

    virtual Float pdf_direct(const PointGeometry& geom, const PointGeometry& geomL, const Transform&, Vec3) const override {
        const auto d  = -geomL.wo;
        const auto at = [&]() {
            const auto at = std::atan2(d.x, d.z);
            return at < 0_f ? at + 2_f * Pi : at;
        }();
        const auto t = (at - rot_) * .5_f / Pi;
        const auto u = t - std::floor(t);
        const auto v = std::acos(d.y) / Pi;
        const auto st = math::safe_sqrt(1 - d.y * d.y);
        if (st == 0_f) {
            return 0_f;
        }
        const auto pdfSA = dist_.pdf(u, v) / (2_f*Pi*Pi*st);
        return surface::convert_pdf_SA_to_projSA(pdfSA, geom, d);
    }

    // --------------------------------------------------------------------------------------------

    virtual bool is_infinite() const override {
        return true;
    }

    virtual bool is_connectable(const PointGeometry&) const override {
        return false;
    }

    virtual Vec3 eval(const PointGeometry& geom, Vec3) const override {
        const auto d = -geom.wo;
        const auto at = [&]() {
            const auto at = std::atan2(d.x, d.z);
            return at < 0_f ? at + 2_f * Pi : at;
        }();
        const auto t = (at - rot_) * .5_f / Pi;
        return envmap_->eval({ t - floor(t), acos(d.y) / Pi });
    }
};

LM_COMP_REG_IMPL(Light_Env, "light::env");

LM_NAMESPACE_END(LM_NAMESPACE)
