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
.. function:: material::mask
   
   Pass-through material.

   This component implements a special material that only sample
   the outgoing ray into the same direction as the incoming ray.
   This material is used to implement texture-masked materials.
   BSDF reads

   .. math::
      f_s(\omega_i, \omega_o) = \delta_\Omega(-\omega_i, \omega_o).
\endrst
*/
class Material_Mask : public Material {
private:
    virtual bool isSpecular(const SurfacePoint& sp) const override {
        LM_UNUSED(sp);
        return true;
    }

    virtual std::optional<RaySample> sampleRay(Rng& rng, const SurfacePoint& sp, Vec3 wi) const {
        LM_UNUSED(rng);
        return RaySample(sp, -wi, Vec3(1_f));
    }
};

LM_COMP_REG_IMPL(Material_Mask, "material::mask");

LM_NAMESPACE_END(LM_NAMESPACE)
