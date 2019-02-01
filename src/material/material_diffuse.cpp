/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/material.h>
#include <lm/texture.h>
#include <lm/json.h>
#include <lm/user.h>
#include <lm/serial.h>
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

    virtual void foreachUnderlying(const ComponentVisitor& visit) override {
        comp::visit(visit, mapKd_);
    }

public:
    virtual bool construct(const Json& prop) override {
        mapKd_ = getAsset<Texture>(prop, "mapKd");
        if (!mapKd_) {
            Kd_ = json::valueOr<Vec3>(prop, "Kd", Vec3(1_f));
        }
        return true;
    }

    virtual bool isSpecular(const PointGeometry&, int) const override {
        return false;
    }

    virtual std::optional<MaterialDirectionSample> sample(Rng& rng, const PointGeometry& geom, Vec3 wi) const override {
        const auto[n, u, v] = geom.orthonormalBasis(wi);
        const auto Kd = mapKd_ ? mapKd_->eval(geom.t) : Kd_;
        const auto d = math::sampleCosineWeighted(rng);
        return MaterialDirectionSample{
            u*d.x + v * d.y + n * d.z,
            SurfaceComp::DontCare,
            Kd
        };
    }

    virtual std::optional<Vec3> reflectance(const PointGeometry& geom, int) const override {
        return mapKd_ ? mapKd_->eval(geom.t) : Kd_;
    }

    virtual Float pdf(const PointGeometry& geom, int, Vec3 wi, Vec3 wo) const override {
        return geom.opposite(wi, wo) ? 0_f : 1_f / Pi;
    }

    virtual Vec3 eval(const PointGeometry& geom, int, Vec3 wi, Vec3 wo) const override {
        if (geom.opposite(wi, wo)) {
            return {};
        }
        const auto a = (mapKd_ && mapKd_->hasAlpha()) ? mapKd_->evalAlpha(geom.t) : 1_f;
        return (mapKd_ ? mapKd_->eval(geom.t) : Kd_) * (a / Pi);
    }
};

LM_COMP_REG_IMPL(Material_Diffuse, "material::diffuse");

LM_NAMESPACE_END(LM_NAMESPACE)
