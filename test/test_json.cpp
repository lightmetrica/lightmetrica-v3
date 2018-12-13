/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
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
            CHECK_THROWS(lm::Vec3 v = "1"_lmJson);
            CHECK_THROWS(lm::Vec3 v = "{}"_lmJson);
            CHECK_THROWS(lm::Vec3 v = "[1,2]"_lmJson);
            CHECK_THROWS(lm::Vec3 v = "[1,2,3,4]"_lmJson);
        }
    }
}

LM_NAMESPACE_END(LM_TEST_NAMESPACE)
