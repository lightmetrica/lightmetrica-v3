/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/lm.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Material_VisualizeNormal final : public Material {
public:
    virtual bool isSpecular(const PointGeometry&, int) const override {
        return false;
    }

    virtual std::optional<MaterialDirectionSample> sample(Rng&, const PointGeometry&, Vec3) const override {
        LM_UNREACHABLE_RETURN();
    }

    virtual std::optional<Vec3> reflectance(const PointGeometry& geom, int) const override {
        return glm::abs(geom.n);
    }

    virtual Float pdf(const PointGeometry&, int, Vec3, Vec3) const override {
        LM_UNREACHABLE_RETURN();
    }

    virtual Float pdfComp(const PointGeometry&, int, Vec3) const override {
        LM_UNREACHABLE_RETURN();
    }

    virtual Vec3 eval(const PointGeometry&, int, Vec3, Vec3) const override {
        LM_UNREACHABLE_RETURN();
    }
};

LM_COMP_REG_IMPL(Material_VisualizeNormal, "material::visualize_normal");
LM_NAMESPACE_END(LM_NAMESPACE)
