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

	virtual std::optional<LightSample> sampleLight(Rng& rng, const SurfacePoint& sp) const override {
		const int i = dist_.samp(rng);
		const auto s = math::safeSqrt(rng.u());
		const auto [a, b, c] = mesh_->triangleAt(i);
		const SurfacePoint spL(
			math::mixBarycentric(a, b, c, Vec2(1_f-s, rng.u()*s)),
			glm::normalize(glm::cross(b - a, c - a)),
			Vec2());
		const auto pp = spL.p - sp.p;
		const auto wo = glm::normalize(pp);
		const auto p = pdfLight(sp, spL, -wo);
		if (p == 0_f) {
			return {};
		}
		const auto Le = eval(sp, -wo);
		return LightSample{ wo, glm::length(pp), Le / p };
	}

	virtual Float pdfLight(const SurfacePoint& sp, const SurfacePoint& spL, Vec3 wo) const override {
		LM_UNUSED(wo);
		const auto G = geometryTerm(sp, spL);
		return G == 0_f ? 0_f : invA_ / G;
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
