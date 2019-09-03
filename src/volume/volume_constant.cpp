/*
	Lightmetrica - Copyright (c) 2019 Hisanari Otsu
	Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/volume.h>
#include <lm/logger.h>
#include <lm/json.h>
#include <lm/serial.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Volume_Constant : public Volume {
private:
	Bound bound_;
	std::optional<Vec3> color_;
	std::optional<Float> scalar_;

public:
	LM_SERIALIZE_IMPL(ar) {
		ar(bound_);
	}

public:
	virtual bool construct(const Json& prop) override {
		bound_.mi = json::value<Vec3>(prop, "bound_min", Vec3(Inf));
		bound_.ma = json::value<Vec3>(prop, "bound_max", Vec3(-Inf));
		color_ = json::valueOrNone<Vec3>(prop, "color");
		scalar_ = json::valueOrNone<Float>(prop, "scalar");
		if (!color_ && !scalar_) {
			LM_ERROR("Either 'color' or 'scalar' property is necessary.");
			return false;
		}
		return true;
	}
	
	virtual Bound bound() const override {
		return bound_;
	}

	virtual bool hasScalar() const override {
		return scalar_.has_value();
	}

	virtual Float maxScalar() const override {
		return *scalar_;
	}

	virtual Float evalScalar(Vec3) const override {
		return *scalar_;
	}

	virtual bool hasColor() const override {
		return color_.has_value();
	}

	virtual Vec3 evalColor(Vec3) const override {
		return *color_;
	}

};

LM_COMP_REG_IMPL(Volume_Constant, "volume::constant");

LM_NAMESPACE_END(LM_NAMESPACE)
