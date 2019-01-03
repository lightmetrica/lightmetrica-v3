/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
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

private:
	Float tranformedInvA(const Transform& transform) const {
		// TODO: Handle degenerated axis
		// e.g., scaling by (.2,.2,.2) leads J=1/5^3
		// but the actual change of areae is J=1/5^2
		return invA_ / transform.J;
	}

public:
    virtual bool construct(const Json& prop) override {
        Ke_ = prop["Ke"];
        mesh_ = parent()->underlying<Mesh>(prop, "mesh");
        
        // Construct CDF for surface sampling
		// Note we construct the CDF before transformation
        mesh_->foreachTriangle([&](int face, Vec3 a, Vec3 b, Vec3 c) {
            LM_UNUSED(face);
            const auto cr = cross(b - a, c - a);
            dist_.add(math::safeSqrt(glm::dot(cr, cr)) * .5_f);
        });
        invA_ = 1_f / dist_.c.back();
        dist_.norm();

        return true;
    }

	virtual std::optional<LightSample> sampleLight(Rng& rng, const SurfacePoint& sp, const Transform& transform) const override {
		const int i = dist_.samp(rng);
		const auto s = math::safeSqrt(rng.u());
		const auto [a, b, c] = mesh_->triangleAt(i);
		const auto p = math::mixBarycentric(a, b, c, Vec2(1_f-s, rng.u()*s));
		const auto n = glm::normalize(glm::cross(b - a, c - a));
		const SurfacePoint spL(
			transform.M * Vec4(p, 1_f),
			transform.normalM * n,
			Vec2());
		const auto ppL = spL.p - sp.p;
		const auto wo = glm::normalize(ppL);
		const auto pL = pdfLight(sp, spL, transform, -wo);
		if (pL == 0_f) {
			return {};
		}
		const auto Le = eval(spL, -wo);
		return LightSample{ wo, glm::length(ppL), Le / pL };
	}

	virtual Float pdfLight(const SurfacePoint& sp, const SurfacePoint& spL, const Transform& transform, Vec3 wo) const override {
		LM_UNUSED(wo);
		const auto G = geometryTerm(sp, spL);
		return G == 0_f ? 0_f : tranformedInvA(transform) / G;
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
