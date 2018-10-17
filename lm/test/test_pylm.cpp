/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include "test_common.h"
#include <generated/test_python.h>
#include <pybind11/embed.h>
#include <lm/detail/pylm.h>

namespace py = pybind11;
using namespace py::literals;

LM_NAMESPACE_BEGIN(LM_TEST_NAMESPACE)

PYBIND11_EMBEDDED_MODULE(test_pylm, m) {
    // Python -> C++
    //m.def("func1", [](lm::json params) -> void {
    //    bool cond = params["cond"];
    //    if (cond)
    //});
}

TEST_CASE("Casting JSON type") {
    
}

LM_NAMESPACE_END(LM_TEST_NAMESPACE)
