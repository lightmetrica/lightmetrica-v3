/*
	Lightmetrica - Copyright (c) 2019 Hisanari Otsu
	Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/volume.h>
#include <lm/json.h>
#include <lm/serial.h>

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
	virtual bool construct(const Json& prop) override {
		bound_.mi = json::value<Vec3>(prop, "bound_min");
		bound_.ma = json::value<Vec3>(prop, "bound_max");
		return true;
	}

	virtual Bound bound() const override {
		return bound_;
	}

	virtual Float maxDensity() const override {
		return 1_f;
	}

	virtual Float evalDensity(Vec3 p) const override {
		const auto Delta = .2_f;
		const auto t = (p - bound_.mi) / (bound_.ma - bound_.mi);
		const auto x = int(t.x / Delta);
		const auto y = int(t.y / Delta);
		//const auto z = int(t.z / Delta);
		return (x + y) % 2 == 0 ? 1_f : 0_f;
	}

	virtual Vec3 evalAlbedo(Vec3) const override {
		//const auto t = (p - b.mi) / (b.ma - b.mi);
		return Vec3(.5_f);
	}
};

LM_COMP_REG_IMPL(Volume_Checker, "volume::checker");

LM_NAMESPACE_END(LM_NAMESPACE)
