/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/pylm.h>

LM_NAMESPACE_BEGIN(lmtest)

PYBIND11_MODULE(pylm_test_module, m) {
    // Simple function
    m.def("test", []() -> int {
        return 42;
    });

    // Round trip
    m.def("round_trip", [](lm::Json v) -> lm::Json {
        return v;
    });
}

LM_NAMESPACE_END(lmtest)
