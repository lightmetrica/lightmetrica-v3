/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include "pylm_test.h"

LM_NAMESPACE_BEGIN(LM_TEST_NAMESPACE)

class PyTestBinder_Json : public PyTestBinder {
public:
    virtual void bind(pybind11::module& m) const {
        m.def("round_trip", [](lm::Json v) -> lm::Json {
            return v;
        });
    }
};

LM_COMP_REG_IMPL(PyTestBinder_Json, "pytestbinder::json");

LM_NAMESPACE_END(LM_TEST_NAMESPACE)
