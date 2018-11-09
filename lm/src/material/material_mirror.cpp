/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/material.h>
#include <lm/scene.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

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
