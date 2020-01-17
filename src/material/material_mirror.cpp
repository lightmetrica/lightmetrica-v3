/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/core.h>
#include <lm/material.h>
#include <lm/surface.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*
\rst
.. function:: material::mirror

   Ideal mirror reflection.

   This component implements ideal mirror reflection BRDF:

   .. math::
      f_r(\omega_i, \omega_o) = \delta_\Omega(\omega_{\mathrm{refl}}, \omega_o),

   where
   :math:`\omega_{\mathrm{refl}}\equiv2(\omega_i\cdot\mathbf{n})\mathbf{n} - \omega_i`
   is the reflected direction of :math:`\omega_i`, and
   :math:`\delta_\Omega` is the Dirac delta function w.r.t. solid angle measure:
   :math:`\int_\Omega \delta_\Omega(\omega', \omega) d\omega = \omega'`.
\endrst
*/
class Material_Mirror final : public Material {
private:
    virtual std::optional<MaterialDirectionSample> sample_direction(Rng&, const PointGeometry& geom, Vec3 wi, MaterialTransDir) const override {
        return MaterialDirectionSample{
            math::reflection(wi, geom.n),
            Vec3(1_f),
            true
        };
    }

    virtual Float pdf_direction(const PointGeometry&, Vec3, Vec3) const override {
        return 0_f;
    }

    virtual Vec3 eval(const PointGeometry&, Vec3, Vec3) const override {
        return Vec3(0_f);
    }

    virtual std::optional<Vec3> reflectance(const PointGeometry&) const override {
        return Vec3(0_f);
    }
};

LM_COMP_REG_IMPL(Material_Mirror, "material::mirror");

LM_NAMESPACE_END(LM_NAMESPACE)
