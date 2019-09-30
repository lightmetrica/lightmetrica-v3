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

    virtual std::optional<LightRaySample> sample(Rng& rng, const PointGeometry& geom, const Transform&) const override {
        const auto wo = math::sampleUniformSphere(rng);
        const auto geomL = PointGeometry::makeInfinite(wo);
        const auto pL = pdf(geom, geomL, 0, {}, wo);
        if (pL == 0_f) {
            return {};
        }
        return LightRaySample{
            geomL,
            wo,
            0,
            Le_ / pL
        };
    }

    virtual Float pdf(const PointGeometry& geom, const PointGeometry& geomL, int, const Transform&, Vec3) const override {
        const auto d = -geomL.wo;
        return surface::convertSAToProjSA(math::pdfUniformSphere(), geom, d);
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
