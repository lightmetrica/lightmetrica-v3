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
.. function:: volume::checker

    Checker volume.

    :param float bound_min: Minimum bound of the volume.
    :param float bound_max: Maximum bound of the volume.
\endrst
*/
class Volume_Checker : public Volume {
private:
	Bound bound_;           // Bound of volume

public:
	LM_SERIALIZE_IMPL(ar) {
		ar(bound_);
	}

public:
	virtual void construct(const Json& prop) override {
		bound_.min = json::value<Vec3>(prop, "bound_min");
		bound_.max = json::value<Vec3>(prop, "bound_max");
	}

	virtual Bound bound() const override {
		return bound_;
	}

	virtual bool has_scalar() const override {
		return true;
	}

	virtual Float max_scalar() const override {
		return 1_f;
	}

	virtual Float eval_scalar(Vec3 p) const override {
		const auto Delta = .2_f;
		const auto t = (p - bound_.min) / (bound_.max - bound_.min);
		const auto x = int(t.x / Delta);
		const auto y = int(t.y / Delta);
		//const auto z = int(t.z / Delta);
		return (x + y) % 2 == 0 ? 1_f : 0_f;
	}

	virtual bool has_color() const override {
		return false;
	}
};

LM_COMP_REG_IMPL(Volume_Checker, "volume::checker");

LM_NAMESPACE_END(LM_NAMESPACE)
