/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include "test_common.h"
#include <lm/json.h>

LM_NAMESPACE_BEGIN(LM_TEST_NAMESPACE)

TEST_CASE("Json") {
    SUBCASE("Conversion of Vec types") {
        SUBCASE("Vec2") {
            SUBCASE("To") {
                lm::Json j = lm::Vec2(1, 2);
                CHECK(j == "[1,2]"_lmJson);
            }
            SUBCASE("From") {
                lm::Vec2 v = "[1,2]"_lmJson;
                CHECK(v == lm::Vec2(1, 2));
            }
        }
        SUBCASE("Vec3") {
            SUBCASE("To") {
                lm::Json j = lm::Vec3(1, 2, 3);
                CHECK(j == "[1,2,3]"_lmJson);
            }
            SUBCASE("From") {
                lm::Vec3 v = "[1,2,3]"_lmJson;
                CHECK(v == lm::Vec3(1, 2, 3));
            }
        }
        SUBCASE("Vec4") {
            SUBCASE("To") {
                lm::Json j = lm::Vec4(1, 2, 3, 4);
                CHECK(j == "[1,2,3,4]"_lmJson);
            }
            SUBCASE("From") {
                lm::Vec4 v = "[1,2,3,4]"_lmJson;
                CHECK(v == lm::Vec4(1, 2, 3, 4));
            }
        }
        SUBCASE("Invalid type") {
            CHECK_THROWS([]{ lm::Vec3 v = "1"_lmJson; }());
            CHECK_THROWS([]{ lm::Vec3 v = "{}"_lmJson; }());
            CHECK_THROWS([]{ lm::Vec3 v = "[1,2]"_lmJson; }());
            CHECK_THROWS([] { lm::Vec3 v = "[1,2,3,4]"_lmJson; }());
        }
    }

    SUBCASE("Conversion of pointer types") {
        SUBCASE("Non const pointer") {
            int v = 42;
            int* vp = &v;
            lm::Json j = vp;
            // User-defined cast operator for pointer types doesn't match the definition.
            // We used get<>() as a workaround.
            auto vp2 = j.get<int*>();
            CHECK(vp == vp2);
        }
        SUBCASE("const pointer") {
            int v = 42;
            const int* vp = &v;
            lm::Json j = vp;
            auto vp2 = j.get<const int*>();
            CHECK(vp == vp2);
        }
    }
}

LM_NAMESPACE_END(LM_TEST_NAMESPACE)
