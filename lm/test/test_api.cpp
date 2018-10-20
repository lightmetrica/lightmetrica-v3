/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include "pch_test.h"
#include "test_common.h"
#include <lm/api.h>

LM_NAMESPACE_BEGIN(LM_TEST_NAMESPACE)

TEST_CASE("primitive")
{
//! [primitive example]
    const int id = lm::api::primitive("test");
    CHECK(id == 42);
//! [primitive example]
}

LM_NAMESPACE_END(LM_TEST_NAMESPACE)
