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
class Material_Mask final : public Material {
public:
#if 0
    virtual bool is_specular(const PointGeometry&, int) const override {
        return true;
    }
#endif

    virtual std::optional<DirectionSample> sample_direction(Rng&, const PointGeometry&, Vec3 wi, TransDir) const override {
        return DirectionSample{
            -wi,
            Vec3(1_f),
            true
        };
    }

    virtual Float pdf_direction(const PointGeometry&, Vec3, Vec3, bool eval_delta) const override {
        return eval_delta ? 0_f : 1_f;
    }

    virtual Vec3 eval(const PointGeometry&, Vec3, Vec3, TransDir, bool eval_delta) const override {
        return eval_delta ? Vec3(0_f) : Vec3(1_f);
    }

    virtual std::optional<Vec3> reflectance(const PointGeometry&) const override {
        return Vec3(0_f);
    }
};

LM_COMP_REG_IMPL(Material_Mask, "material::mask");

LM_NAMESPACE_END(LM_NAMESPACE)
