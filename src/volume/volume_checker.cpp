/*
	Lightmetrica - Copyright (c) 2019 Hisanari Otsu
	Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/core.h>
#include <lm/volume.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// Memo:
// Currently we forget about muA. Assume muA=0
// density = muT = muS
class Volume_Checker : public Volume {
private:
	Bound bound_;           // Bound of volume

public:
	LM_SERIALIZE_IMPL(ar) {
		ar(bound_);
	}

public:
	virtual void construct(const Json& prop) override {
		bound_.mi = json::value<Vec3>(prop, "bound_min");
		bound_.ma = json::value<Vec3>(prop, "bound_max");
	}

	virtual Bound bound() const override {
		return bound_;
	}

	virtual bool hasScalar() const override {
		return true;
	}

	virtual Float maxScalar() const override {
		return 1_f;
	}

	virtual Float evalScalar(Vec3 p) const override {
		const auto Delta = .2_f;
		const auto t = (p - bound_.mi) / (bound_.ma - bound_.mi);
		const auto x = int(t.x / Delta);
		const auto y = int(t.y / Delta);
		//const auto z = int(t.z / Delta);
		return (x + y) % 2 == 0 ? 1_f : 0_f;
	}

	virtual bool hasColor() const override {
		return false;
	}
};

LM_COMP_REG_IMPL(Volume_Checker, "volume::checker");

LM_NAMESPACE_END(LM_NAMESPACE)
