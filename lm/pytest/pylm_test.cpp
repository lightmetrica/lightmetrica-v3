/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include "pylm_test.h"

LM_NAMESPACE_BEGIN(LM_TEST_NAMESPACE)

PYBIND11_MODULE(pylm_test, m) {
    m.doc() = "Lightmetrica python test module";

    // Initialize submodules
    std::regex reg(R"(pytestbinder::(\w+))");
    lm::comp::detail::foreachRegistered([&](const std::string& name) {
        // Find the name matched with 'pytestbinder::*'
        std::smatch match;
        if (std::regex_match(name, match, reg)) {
            auto binder = lm::comp::create<PyTestBinder>(name, nullptr);
            auto submodule = m.def_submodule(match[1].str().c_str());
            binder->bind(submodule);
        }
    });
}

LM_NAMESPACE_END(LM_TEST_NAMESPACE)
