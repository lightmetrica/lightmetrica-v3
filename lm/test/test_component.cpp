/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include "test_common.h"

LM_NAMESPACE_BEGIN(LM_TEST_NAMESPACE)

struct A : public lm::Component {
    virtual int f1(int a) = 0;
    virtual int f2(int a, int b) = 0;
};

struct A1 final : public A {
    virtual int f1(int a) {
        return 42;
    }
    virtual int f2(int a, int b) {
        return a + b;
    }
};

LM_COMPONENT_REGISTER_IMPL(A1, "test::comp::a1");

TEST_CASE("component")
{
    SUBCASE("simple cases") {
        const auto p = lm::comp::create<A>("test::comp::a1");
        CHECK(p->f1() == 42);
        CHECK(p->f2(1, 2) == 3);
    }
}

LM_NAMESPACE_END(LM_TEST_NAMESPACE)