/*
    Lightmetrica V3 Prototype
    Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/detail/pylm.h>

LM_NAMESPACE_BEGIN(lmtest)

PYBIND11_MODULE(pylm_test_module, m) {
    // Round trip
    m.def("roundtrip", [](lm::json v) -> lm::json {
        return v;
    });
}

LM_NAMESPACE_END(lmtest)
