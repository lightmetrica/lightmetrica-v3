/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include "test_common.h"
#include <lm/exception.h>

#if LM_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable:4723)  // potential divide by 0
#endif

LM_NAMESPACE_BEGIN(LM_TEST_NAMESPACE)

TEST_CASE("Exception") {
    lm::log::ScopedInit log_;
    lm::exception::ScopedInit ex_;

    const auto Check = [](const std::function<void()>& func) -> std::string {
        try { func(); }
        catch (const std::runtime_error& e) { return e.what(); }
        return {};
    };

    // ------------------------------------------------------------------------

    SUBCASE("Supported") {
        SUBCASE("Multiply inftinity and zero") {
            const auto code = Check([]() {
                // We put volatile keyword to supress optimization
                const volatile double z = 0;
                const volatile double t = std::numeric_limits<double>::infinity() * z;
                LM_UNUSED(t);
            });
            CHECK(code == "EXCEPTION_FLT_INVALID_OPERATION");
        }

        SUBCASE("Divide zero by zero") {
            const auto code = Check([]() {
                const volatile double z = 0;
                const volatile double t = 0 / z;
                LM_UNUSED(t);
            });
            CHECK(code == "EXCEPTION_FLT_INVALID_OPERATION");
        }

        SUBCASE("Divide by zero") {
            const auto code = Check([]() {
                const volatile double z = 0;
                const volatile double t = 1.0 / z;
                LM_UNUSED(t);
            });
            CHECK(code == "EXCEPTION_FLT_DIVIDE_BY_ZERO");
        }

        SUBCASE("Square root of -1") {
            const auto code = Check([]() {
                const volatile double t = std::sqrt(-1);
                LM_UNUSED(t);
            });
            CHECK(code == "EXCEPTION_FLT_INVALID_OPERATION");
        }

        #if 0
        // Temporarily ignored this test case due to the bug in VS2017
        // https://developercommunity.visualstudio.com/content/problem/155064/stdnumeric-limitssignaling-nan-returns-quiet-nan.html
        SUBCASE("Signaling NaN") {
            const auto code = Check([]() {
                const volatile double one = 1.0;
                const volatile double nan = std::numeric_limits<double>::signaling_NaN();
                const volatile double t = one * nan;
                LM_UNUSED(t);
            });
            CHECK(code == "EXCEPTION_FLT_INVALID_OPERATION");
        }
        #endif
    }

    // ------------------------------------------------------------------------
}

LM_NAMESPACE_END(LM_TEST_NAMESPACE)

#if LM_COMPILER_MSVC
#pragma warning(pop)
#endif
