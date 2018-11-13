/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include "pylm_test.h"

LM_NAMESPACE_BEGIN(LM_TEST_NAMESPACE)

class PyTestBinder_Math : public PyTestBinder {
public:
    virtual void bind(pybind11::module& m) const {
        m.def("compSum3", [](lm::Vec3 v) -> lm::Float {
            return v.x + v.y + v.z;
        });
    }
};

LM_COMP_REG_IMPL(PyTestBinder_Math, "pytestbinder::math");

LM_NAMESPACE_END(LM_TEST_NAMESPACE)
