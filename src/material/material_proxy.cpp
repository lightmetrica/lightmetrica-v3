/*
	Lightmetrica - Copyright (c) 2018 Hisanari Otsu
	Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/material.h>
#include <lm/scene.h>
#include <lm/user.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Material_Proxy : public Material {
private:
	Material* ref_;

public:
	virtual bool construct(const Json& prop) override {
		ref_ = dynamic_cast<Material*>(lm::getAsset(prop["ref"]));
		return ref_;
	}

	virtual bool isSpecular(const SurfacePoint& sp) const override {
		return ref_->isSpecular(sp);
	}

	virtual std::optional<RaySample> sampleRay(Rng& rng, const SurfacePoint& sp, Vec3 wi) const override {
		return ref_->sampleRay(rng, sp, wi);
	}

	virtual std::optional<Vec3> reflectance(const SurfacePoint& sp) const override {
		return ref_->reflectance(sp);
	}

	Float pdf(const SurfacePoint& sp, Vec3 wi, Vec3 wo) const override {
		return ref_->pdf(sp, wi, wo);
	}

	virtual Vec3 eval(const SurfacePoint& sp, Vec3 wi, Vec3 wo) const override {
		return ref_->eval(sp, wi, wo);
	}
};

LM_COMP_REG_IMPL(Material_Proxy, "material::proxy");

LM_NAMESPACE_END(LM_NAMESPACE)
