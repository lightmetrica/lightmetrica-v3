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
    virtual ComponentSample sample_component(const ComponentSampleU&, const PointGeometry&) const override {
        return { 0, 1_f };
    }

    virtual Float pdf_component(int, const PointGeometry&) const override {
        return 1_f;
    }

    virtual std::optional<DirectionSample> sample_direction(const DirectionSampleU&, const PointGeometry&, Vec3 wi, int, TransDir) const override {
        return DirectionSample{
            -wi,
            Vec3(1_f)
        };
    }

    virtual Float pdf_direction(const PointGeometry&, Vec3, Vec3, int, bool eval_delta) const override {
        return eval_delta ? 0_f : 1_f;
    }

    virtual Vec3 eval(const PointGeometry&, Vec3, Vec3, int, TransDir, bool eval_delta) const override {
        return eval_delta ? Vec3(0_f) : Vec3(1_f);
    }

    virtual std::optional<Vec3> reflectance(const PointGeometry&) const override {
        return Vec3(0_f);
    }

    virtual bool is_specular_component(int) const override {
        return true;
    }
};

LM_COMP_REG_IMPL(Material_Mask, "material::mask");

LM_NAMESPACE_END(LM_NAMESPACE)
