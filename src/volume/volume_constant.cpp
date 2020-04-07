/*
	Lightmetrica - Copyright (c) 2019 Hisanari Otsu
	Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/core.h>
#include <lm/volume.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*
\rst
.. function:: volume::constant

    Constant volume.

    :param color color: Stored color value.
    :param float scalar: Stored scalar value.
    :param float bound_min: Minimum bound of the volume.
    :param float bound_max: Maximum bound of the volume.
\endrst
*/
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
	virtual void construct(const Json& prop) override {
		bound_.min = json::value<Vec3>(prop, "bound_min", Vec3(Inf));
		bound_.max = json::value<Vec3>(prop, "bound_max", Vec3(-Inf));
		color_ = json::value_or_none<Vec3>(prop, "color");
		scalar_ = json::value_or_none<Float>(prop, "scalar");
		if (!color_ && !scalar_) {
            LM_THROW_EXCEPTION(Error::InvalidArgument,
			    "Either 'color' or 'scalar' property is necessary.");
		}
	}
	
	virtual Bound bound() const override {
		return bound_;
	}

	virtual bool has_scalar() const override {
		return scalar_.has_value();
	}

	virtual Float max_scalar() const override {
		return *scalar_;
	}

	virtual Float eval_scalar(Vec3) const override {
		return *scalar_;
	}

	virtual bool has_color() const override {
		return color_.has_value();
	}

	virtual Vec3 eval_color(Vec3) const override {
		return *color_;
	}
};

LM_COMP_REG_IMPL(Volume_Constant, "volume::constant");

LM_NAMESPACE_END(LM_NAMESPACE)
