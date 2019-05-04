/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/lm.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Material_VisualizeNormal final : public Material {
public:
    virtual bool construct(const Json&) override {
        return true;
    }
    virtual std::optional<Vec3> reflectance(const PointGeometry& geom, int) const override {
        return glm::abs(geom.n);
    }
};

LM_COMP_REG_IMPL(Material_VisualizeNormal, "material::visualize_normal");
LM_NAMESPACE_END(LM_NAMESPACE)
