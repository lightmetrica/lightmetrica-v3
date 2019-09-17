/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/core.h>
#include <lm/light.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
\rst
.. function:: light::envconst

    Constant environment light.

    :param color Le: Luminance.
\endrst
*/
class Light_EnvConst final : public Light {
private:
    Vec3 Le_;
    
public:
    LM_SERIALIZE_IMPL(ar) {
        ar(Le_);
    }
    
public:
    virtual bool construct(const Json& prop) override {
        Le_ = json::value<Vec3>(prop, "Le");
        return true;
    }

    virtual std::optional<LightRaySample> sample(Rng& rng, const PointGeometry&, const Transform&) const override {
        const auto wo = math::sampleUniformSphere(rng);
        const auto pL = math::pdfUniformSphere();
        return LightRaySample{
            PointGeometry::makeInfinite(wo),
            wo,
            0,
            Le_ / pL
        };
    }

    virtual Float pdf(const PointGeometry&, const PointGeometry&, int, const Transform&, Vec3) const override {
        return math::pdfUniformSphere();
    }

    virtual bool isSpecular(const PointGeometry&, int) const override {
        return false;
    }

    virtual bool isInfinite() const override {
        return true;
    }

    virtual Vec3 eval(const PointGeometry&, int, Vec3) const override {
        return Le_;
    }
};

LM_COMP_REG_IMPL(Light_EnvConst, "light::envconst");

LM_NAMESPACE_END(LM_NAMESPACE)
