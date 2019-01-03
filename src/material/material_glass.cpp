/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/material.h>
#include <lm/scene.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Material_Glass : public Material {
private:
    Float Ni_;

public:
    virtual bool construct(const Json& prop) override {
        Ni_ = prop["Ni"];
        return true;
    }

    virtual bool isSpecular(const SurfacePoint& sp) const override {
        LM_UNUSED(sp);
        return true;
    }

    virtual std::optional<RaySample> sampleRay(Rng& rng, const SurfacePoint& sp, Vec3 wi) const {
        const bool in = glm::dot(wi, sp.n) > 0_f;
        const auto n = in ? sp.n : -sp.n;
        const auto eta = in ? 1_f / Ni_ : Ni_;
        auto wt = math::refraction(wi, n, eta);
        const auto Fr = !wt ? 1_f : [&]() {
            // Flesnel term
            const auto cos = in ? glm::dot(wi, sp.n) : glm::dot(*wt, sp.n);
            const auto r = (1_f-Ni_) / (1_f+Ni_);
            const auto r2 = r*r;
            return r2 + (1_f - r2) * std::pow(1_f-cos, 5_f);
        }();
        if (rng.u() < Fr) {
            // Reflection
            return RaySample(
                sp,
                math::reflection(wi, sp.n),
                Vec3(1_f)
            );
        }
        // Refraction
        return RaySample(
            sp,
            *wt,
            Vec3(eta*eta)
        );
    }
};

LM_COMP_REG_IMPL(Material_Glass, "material::glass");

LM_NAMESPACE_END(LM_NAMESPACE)
