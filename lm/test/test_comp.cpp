/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include "test_common.h"
#include "test_interface.h"

LM_NAMESPACE_BEGIN(LM_TEST_NAMESPACE)

// ----------------------------------------------------------------------------

// _begin_snippet: A
struct A : public lm::Component {
    virtual int f1() = 0;
    virtual int f2(int a, int b) = 0;
};

struct A1 final : public A {
    virtual int f1() { return 42; }
    virtual int f2(int a, int b) { return a + b; }
};

LM_COMP_REG_IMPL(A1, "test::comp::a1");
// _end_snippet: A

// ----------------------------------------------------------------------------

struct B : public A {
    virtual int f3() = 0;
};

struct B1 final : public B {
    virtual int f1() { return 42; }
    virtual int f2(int a, int b) { return a + b; }
    virtual int f3() { return 43; }
};

LM_COMP_REG_IMPL(B1, "test::comp::b1");

// ----------------------------------------------------------------------------

struct C : public lm::Component {
    C() { std::cout << "C"; }
    virtual ~C() { std::cout << "~C"; }
};

struct C1 : public C {
    C1() { std::cout << "C1"; }
    virtual ~C1() { std::cout << "~C1"; }
};

LM_COMP_REG_IMPL(C1, "test::comp::c1");

// ----------------------------------------------------------------------------

TEST_CASE("Component")
{
    SUBCASE("Simple interface") {
        // _begin_snippet: A_impl
        const auto p = lm::comp::create<A>("test::comp::a1");
        REQUIRE(p);
        CHECK(p->f1() == 42);
        CHECK(p->f2(1, 2) == 3);
        // _end_snippet: A_impl
    }
    SUBCASE("Inherited interface") {
        const auto p = lm::comp::create<B>("test::comp::b1");
        REQUIRE(p);
        CHECK(p->f1() == 42);
        CHECK(p->f2(1, 2) == 3);
        CHECK(p->f3() == 43);
    }
    SUBCASE("Missing implementation") {
        const auto p = lm::comp::create<A>("test::comp::a_missing");
        CHECK(p == nullptr);
    }
    SUBCASE("Cast to parent interface") {
        auto b = lm::comp::create<B>("test::comp::b1");
        const auto a = lm::comp::UniquePtr<A>(std::move(b));
        REQUIRE(a);
        CHECK(a->f1() == 42);
        CHECK(a->f2(1, 2) == 3);
    }
    SUBCASE("Constructor and destructor") {
        const auto out = captureStdout([]() {
            const auto p = lm::comp::create<C>("test::comp::c1");
            REQUIRE(p);
        });
        CHECK(out == "CC1~C1~C");
    }
    SUBCASE("Plugin") {
        REQUIRE(lm::comp::detail::loadPlugin("lm_test_plugin"));
        SUBCASE("Simple") {
            auto p = lm::comp::create<lm::TestPlugin>("testplugin::default");
            REQUIRE(p);
            CHECK(p->f() == 42);
        }
        SUBCASE("Plugin constructor and destructor") {
            const auto out = captureStdout([]() {
                const auto p = lm::comp::create<lm::TestPluginWithCtorAndDtor>("testpluginxtor::default");
                REQUIRE(p);
            });
            CHECK(out == "AB~B~A");
        }
        lm::comp::detail::unloadPlugins();
    }
    SUBCASE("Failed to load plugin") {
        REQUIRE(!lm::comp::detail::loadPlugin("__missing_plugin_name__"));
    }
}

LM_NAMESPACE_END(LM_TEST_NAMESPACE)
