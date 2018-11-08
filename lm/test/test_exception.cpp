/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include "test_common.h"
#include <lm/exception.h>

LM_NAMESPACE_BEGIN(LM_TEST_NAMESPACE)

TEST_CASE("Exception") {
    lm::log::ScopedInit log_;
    lm::exception::ScopedInit ex_;

    // ------------------------------------------------------------------------

    const auto Check = [](const std::function<void()>& func) -> std::string {
        try {
            func();
        }
        catch (const std::runtime_error& e) {
            return e.what();
        }
        return "";
    };

    // ------------------------------------------------------------------------

    SUBCASE("Infty * zero") {
        const auto code = Check([]() {
            const volatile double t = std::numeric_limits<double>::infinity() * 0;
            LM_UNUSED(t);
        });
        CHECK(code == "EXCEPTION_FLT_INVALID_OPERATION");
    }

    SUBCASE("Divide by zero") {
        const auto code = Check([]() {
            double z = 0;
            const volatile double t = 0 / z;
            LM_UNUSED(t);
        });
        CHECK(code == "EXCEPTION_FLT_INVALID_OPERATION");
    }
}

LM_NAMESPACE_END(LM_TEST_NAMESPACE)
