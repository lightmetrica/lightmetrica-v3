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

    const auto Check = [](const std::function<void()>& func) -> std::string {
        try { func(); }
        catch (const std::runtime_error& e) { return e.what(); }
        return {};
    };

    // ------------------------------------------------------------------------

    SUBCASE("Supported") {
        SUBCASE("Multiply inftinity and zero") {
            const auto f = []() {
                // We put volatile keyword to supress optimization
                const volatile double z = 0;
                const volatile double t = std::numeric_limits<double>::infinity() * z;
                LM_UNUSED(t);
            };
            SUBCASE("Enabled") {
                lm::exception::ScopedInit ex_;
                CHECK(Check(f) == "EXCEPTION_FLT_INVALID_OPERATION");
            }
            SUBCASE("Disabled") {
                CHECK(Check(f).empty());
            }
        }

        SUBCASE("Divide zero by zero") {
            const auto f = []() {
                const volatile double z = 0;
                const volatile double t = 0 / z;
                LM_UNUSED(t);
            };
            SUBCASE("Enabled") {
                lm::exception::ScopedInit ex_;
                CHECK(Check(f) == "EXCEPTION_FLT_INVALID_OPERATION");
            }
            SUBCASE("Disabled") {
                CHECK(Check(f).empty());
            }
        }

        SUBCASE("Divide by zero") {
            const auto f = []() {
                const volatile double z = 0;
                const volatile double t = 1.0 / z;
                LM_UNUSED(t);
            };
            SUBCASE("Enabled") {
                lm::exception::ScopedInit ex_;
                CHECK(Check(f) == "EXCEPTION_FLT_DIVIDE_BY_ZERO");
            }
            SUBCASE("Disabled") {
                CHECK(Check(f).empty());
            }
        }

        SUBCASE("Square root of -1") {
            const auto f = []() {
                const volatile double t = std::sqrt(-1);
                LM_UNUSED(t);
            };
            SUBCASE("Enabled") {
                lm::exception::ScopedInit ex_;
                CHECK(Check(f) == "EXCEPTION_FLT_INVALID_OPERATION");
            }
            SUBCASE("Disabled") {
                CHECK(Check(f).empty());
            }
        }

        #if 0
        // Temporarily ignored this test case due to the bug in VS2017
        // https://developercommunity.visualstudio.com/content/problem/155064/stdnumeric-limitssignaling-nan-returns-quiet-nan.html
        SUBCASE("Signaling NaN") {
            const auto f = []() {
                const volatile double one = 1.0;
                const volatile double nan = std::numeric_limits<double>::signaling_NaN();
                const volatile double t = one * nan;
                LM_UNUSED(t);
            };
            SUBCASE("Enabled") {
                lm::exception::ScopedInit ex_;
                CHECK(Check(f) == "EXCEPTION_FLT_INVALID_OPERATION");
            }
            SUBCASE("Disabled") {
                CHECK(Check(f).empty());
            }
        }
        #endif
    }

    // ------------------------------------------------------------------------

    SUBCASE("Unsupported") {
        lm::exception::ScopedInit ex_;

        SUBCASE("Denormal") {
            // 4.940656e-324 : denormal
            const double t = 4.940656e-324;
            CHECK(std::fpclassify(t) == FP_SUBNORMAL);
        }

        SUBCASE("Below denormal") {
            // 4.940656e-325 : below representable number by denormal -> clamped to zero
            const double t = 4.940656e-325;
            CHECK(std::fpclassify(t) == FP_ZERO);
        }

        SUBCASE("Inexact 1") {
            const auto code = Check([]() {
                const volatile double t = 2.0 / 3.0;
                LM_UNUSED(t);
            });
            CHECK(code.empty());
        }

        SUBCASE("Inexact 2") {
            const auto code = Check([]() {
                const volatile double t = std::log(1.1);
                LM_UNUSED(t);
            });
            CHECK(code.empty());
        }
    }
}

LM_NAMESPACE_END(LM_TEST_NAMESPACE)

#if LM_COMPILER_MSVC
#pragma warning(pop)
#endif
