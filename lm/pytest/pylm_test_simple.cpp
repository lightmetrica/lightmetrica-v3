/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include "pylm_test.h"

LM_NAMESPACE_BEGIN(LM_TEST_NAMESPACE)

class PyTestBinder_Simple : public PyTestBinder {
public:
    virtual void bind(pybind11::module& m) const {
        m.def("test", []() -> int {
            return 42;
        });
    }
};

LM_COMP_REG_IMPL(PyTestBinder_Simple, "pytestbinder::simple");

LM_NAMESPACE_END(LM_TEST_NAMESPACE)
