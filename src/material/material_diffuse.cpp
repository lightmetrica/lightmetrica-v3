/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/core.h>
#include <lm/material.h>
#include <lm/texture.h>
#include <lm/surface.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*
\rst
.. function:: material::diffuse

    Lambertian diffuse model.

    :param str mapKd: Diffuse reflectance as texture specified by
                        asset name or locator.
    :param color Kd:  Diffuse reflectance as color value.
                        If both ``mapKd`` and ``Kd`` are specified,
                        ``mapKd`` has priority. Default: ``[1,1,1]``.

    This component implements Lambertian diffuse BRDF defined as
   
    .. math:: f_r(\omega_i, \omega_o) = \frac{\rho}{\pi},

    where :math:`\rho` is diffuse reflectance.
\endrst
*/
class Material_Diffuse final : public Material {
private:
    Vec3 Kd_;
    Texture* mapKd_ = nullptr;

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(Kd_, mapKd_);
    }

    virtual Component* underlying(const std::string& name) const override {
        if (name == "mapKd") {
            return mapKd_;
        }
        return nullptr;
    }

    virtual void foreach_underlying(const ComponentVisitor& visit) override {
        comp::visit(visit, mapKd_);
    }

public:
    virtual void construct(const Json& prop) override {
        mapKd_ = json::comp_ref_or_nullptr<Texture>(prop, "mapKd");
        Kd_ = json::value(prop, "Kd", Vec3(1_f));
    }

    virtual ComponentSample sample_component(const ComponentSampleU&, const PointGeometry&, Vec3) const override {
        return { 0, 1_f };
    }

    virtual Float pdf_component(int, const PointGeometry&, Vec3) const override {
        return 1_f;
    }

    virtual std::optional<DirectionSample> sample_direction(const DirectionSampleU& us, const PointGeometry& geom, Vec3 wi, int, TransDir) const override {
        const auto[n, u, v] = geom.orthonormal_basis_twosided(wi);
        const auto Kd = mapKd_ ? mapKd_->eval(geom.t) : Kd_;
        const auto d = math::sample_cosine_weighted(us.ud);
        return DirectionSample{
            u*d.x + v * d.y + n * d.z,
            Kd
        };
    }

    virtual Vec3 reflectance(const PointGeometry& geom) const override {
        return mapKd_ ? mapKd_->eval(geom.t) : Kd_;
    }

    virtual Float pdf_direction(const PointGeometry& geom, Vec3 wi, Vec3 wo, int, bool) const override {
        return geom.opposite(wi, wo) ? 0_f : 1_f / Pi;
    }

    virtual Vec3 eval(const PointGeometry& geom, Vec3 wi, Vec3 wo, int, TransDir, bool) const override {
        if (geom.opposite(wi, wo)) {
            return {};
        }
        return (mapKd_ ? mapKd_->eval(geom.t) : Kd_) / Pi;
    }

    virtual bool is_specular_component(int) const override {
        return false;
    }
};

LM_COMP_REG_IMPL(Material_Diffuse, "material::diffuse");

LM_NAMESPACE_END(LM_NAMESPACE)
