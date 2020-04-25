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
.. function:: volume::gaussian

    Gaussian volume.

    :param color color: Stored color value.
    :param float scalar: Stored max scalar value.
    :param Vec3 pos: Center position of the volume
    :param Vec3 sigma: Gaussian parameter of standard deviation for x,y,z axis
\endrst
*/
class Volume_Gaussian : public Volume {
private:
	Bound bound_;// Computed from 0.001% of the max of gaussian, then times two
	std::optional<Vec3> color_;
	std::optional<Float> scalar_;
	Vec3 pos_;
	Vec3 sigma_;

public:
	LM_SERIALIZE_IMPL(ar) {
		ar(bound_);
	}

public:
	virtual void construct(const Json& prop) override {
		color_ = json::value_or_none<Vec3>(prop, "color");
		scalar_ = json::value_or_none<Float>(prop, "scalar");
		pos_ = json::value<Vec3>(prop, "pos", Vec3(.0_f, .0_f, .0_f));
		sigma_ = json::value<Vec3>(prop, "sigma", Vec3(1._f, 1._f, 1._f));

		if ( !color_ && !scalar_ ) {
			LM_THROW_EXCEPTION(Error::InvalidArgument,
				"Either 'color' or 'scalar' property is necessary.");
		}

		bound_ = computeBound(sigma_);
	}

    Bound computeBound(const Vec3& sigma) const {
		auto compAC=[&](Float s) -> Float {
			//the first 2._f is empirical: when scalar is large, Eps is not enough
			return 2._f*sqrt(-2._f*s*std::log(Eps));};
		Float ac = std::max( std::max(compAC(sigma.x),compAC(sigma.y)),compAC(sigma.z) );
		Bound b;
		b.max = pos_+ac;
		b.min = pos_-ac;

		LM_DEBUG("min bound: {}, {}, {}", b.min.x, b.min.y, b.min.z);
		LM_DEBUG("max bound: {}, {}, {}", b.max.x, b.max.y, b.max.z);
		return b;
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

    Float gaussian(const Vec3& p, const Float max_v, const Vec3& s) const{
        return max_v*exp(- 0.5_f*( (p.x*p.x)/(s.x*s.x) + (p.y*p.y)/(s.y*s.y) + (p.z*p.z)/(s.z*s.z) ) );
    }

	virtual Float eval_scalar(Vec3 p) const override {
		return gaussian(pos_-p, max_scalar(), sigma_);
	}

	virtual bool has_color() const override {
		return color_.has_value();
	}

	virtual Vec3 eval_color(Vec3) const override {
		return *color_;
	}
};

LM_COMP_REG_IMPL(Volume_Gaussian, "volume::gaussian");

LM_NAMESPACE_END(LM_NAMESPACE)
