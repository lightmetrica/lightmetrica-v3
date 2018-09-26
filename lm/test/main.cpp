/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/lm.h>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

TEST_CASE("primitive")
{
//! [primitive example]
    const int id = lm::primitive("test");
    CHECK(id == 42);
//! [primitive example]
}