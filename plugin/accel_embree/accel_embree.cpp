/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/accel.h>
#include <lm/scene.h>
#include <lm/exception.h>
#include <embree3/rtcore.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*
\rst
.. function:: accel::embree

   Acceleration structure with Embree library.
\endrst
*/
class Accel_Embree final : public Accel {
private:

public:
    virtual void build(const Scene& scene) override {
        LM_UNUSED(scene);
    }

    virtual std::optional<Hit> intersect(Ray ray, Float tmin, Float tmax) const override {
        LM_UNUSED(ray, tmin, tmax);
        return {};
    }
};

LM_COMP_REG_IMPL(Accel_Embree, "accel::embree");

LM_NAMESPACE_END(LM_NAMESPACE)
