/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/phase.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Phase_Isotropic final : public Phase {
public:
    virtual bool isSpecular(const PointGeometry&) const override {
        return false;
    }

    virtual std::optional<PhaseDirectionSample> sample(Rng& rng, const PointGeometry&, Vec3) const override {
        return PhaseDirectionSample{
            math::sampleUniformSphere(rng),
            Vec3(1_f)
        };
    }

    virtual Float pdf(const PointGeometry&, Vec3, Vec3) const override {
        return math::pdfUniformSphere();
    }

    virtual Vec3 eval(const PointGeometry&, Vec3, Vec3) const override {
        // Normalization constant = 1/(4*pi)
        return Vec3(math::pdfUniformSphere());
    }
};

LM_COMP_REG_IMPL(Phase_Isotropic, "phase::isotropic");

LM_NAMESPACE_END(LM_NAMESPACE)
