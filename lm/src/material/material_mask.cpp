/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/material.h>
#include <lm/scene.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Material_Mask : public Material {
private:
    virtual bool isSpecular() const override {
        return true;
    }

    virtual std::optional<RaySample> sampleRay(Rng& rng, const SurfacePoint& sp, Vec3 wi) const {
        return RaySample(sp, -wi, Vec3(1_f));
    }
};

LM_COMP_REG_IMPL(Material_Mask, "material::mask");

LM_NAMESPACE_END(LM_NAMESPACE)
