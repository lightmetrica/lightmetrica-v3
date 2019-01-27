/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/material.h>
#include <lm/scene.h>

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
class Material_Mirror : public Material {
private:
    virtual bool isSpecular(const SurfacePoint& sp) const override {
        LM_UNUSED(sp);
        return true;
    }

    virtual std::optional<RaySample> sampleRay(Rng& rng, const SurfacePoint& sp, Vec3 wi) const {
        LM_UNUSED(rng);
        return RaySample(sp, math::reflection(wi, sp.n), Vec3(1_f));
    }
};

LM_COMP_REG_IMPL(Material_Mirror, "material::mirror");

LM_NAMESPACE_END(LM_NAMESPACE)
