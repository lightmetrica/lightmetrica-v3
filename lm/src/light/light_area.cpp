/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/light.h>
#include <lm/mesh.h>
#include <lm/scene.h>
#include <lm/json.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Light_Area : public Light {
private:
    Vec3 Ke_;           // Luminance
    Dist dist_;         // For surface sampling of area lights
    Float invA_;        // Inverse area of area lights
    const Mesh* mesh_;  // Underlying mesh

public:
    virtual bool construct(const Json& prop) override {
        Ke_ = prop["Ke"];
        mesh_ = parent()->underlying<Mesh>(prop, "mesh");
        
        // Construct CDF for surface sampling
        mesh_->foreachTriangle([&](int face, Vec3 a, Vec3 b, Vec3 c) {
            LM_UNUSED(face);
            const auto cr = cross(b - a, c - a);
            dist_.add(math::safeSqrt(glm::dot(cr, cr)) * .5_f);
        });
        invA_ = 1_f / dist_.c.back();
        dist_.norm();

        return true;
    }

    virtual bool isSpecular(const SurfacePoint& sp) const override {
        LM_UNUSED(sp);
        return false;
    }

    virtual Vec3 eval(const SurfacePoint& sp, Vec3 wo) const override {
        return glm::dot(wo, sp.n) <= 0_f ? Vec3(0_f) : Ke_;
    }
};

LM_COMP_REG_IMPL(Light_Area, "light::area");

LM_NAMESPACE_END(LM_NAMESPACE)
