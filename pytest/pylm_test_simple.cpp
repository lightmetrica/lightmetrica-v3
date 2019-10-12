/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch_pylm.h>
#include <lm/pylm.h>

namespace py = pybind11;
using namespace py::literals;

LM_NAMESPACE_BEGIN(lmtest)

class PyTestBinder_Simple : public lm::PyBinder {
public:
    virtual void bind(py::module& m) const {
        m.def("test", []() -> int {
            return 42;
        });
    }
};

LM_COMP_REG_IMPL(PyTestBinder_Simple, "pytestbinder::simple");

LM_NAMESPACE_END(lmtest)
