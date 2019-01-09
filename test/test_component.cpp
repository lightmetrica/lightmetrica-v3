/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include "test_common.h"
#define LM_TEST_INTERFACE_REG_IMPL
#include "test_interface.h"

LM_NAMESPACE_BEGIN(LM_TEST_NAMESPACE)

// ----------------------------------------------------------------------------

TEST_CASE("Component") {
    lm::log::ScopedInit init;

    SUBCASE("Simple interface") {
        // _begin_snippet: A_impl
        const auto p = lm::comp::create<A>("test::comp::a1", nullptr);
        REQUIRE(p);
        CHECK(p->f1() == 42);
        CHECK(p->f2(1, 2) == 3);
        // _end_snippet: A_impl
    }

    SUBCASE("Inherited interface") {
        const auto p = lm::comp::create<B>("test::comp::b1", nullptr);
        REQUIRE(p);
        CHECK(p->f1() == 42);
        CHECK(p->f2(1, 2) == 3);
        CHECK(p->f3() == 43);
    }

    SUBCASE("Missing implementation") {
        const auto p = lm::comp::create<A>("test::comp::a_missing", nullptr);
        CHECK(p == nullptr);
    }

    SUBCASE("Cast to parent interface") {
        auto b = lm::comp::create<B>("test::comp::b1", nullptr);
        const auto a = std::unique_ptr<A>(b.release());
        REQUIRE(a);
        CHECK(a->f1() == 42);
        CHECK(a->f2(1, 2) == 3);
    }

    SUBCASE("Constructor and destructor") {
        const auto out = captureStdout([]() {
            const auto p = lm::comp::create<C>("test::comp::c1", nullptr);
            REQUIRE(p);
        });
        CHECK(out == "CC1~C1~C");
    }

    SUBCASE("Plugin") {
        lm::comp::detail::ScopedLoadPlugin pluginGuard("lm_test_plugin");
        REQUIRE(pluginGuard.valid());
        SUBCASE("Simple") {
            auto p = lm::comp::create<TestPlugin>("testplugin::default", nullptr);
            REQUIRE(p);
            CHECK(p->f() == 42);
        }
        SUBCASE("Plugin constructor and destructor") {
            const auto out = captureStdout([]() {
                const auto p = lm::comp::create<TestPluginWithCtorAndDtor>("testpluginxtor::default", nullptr);
                REQUIRE(p);
            });
            CHECK(out == "AB~B~A");
        }
    }

    SUBCASE("Failed to load plugin") {
        REQUIRE(!lm::comp::detail::loadPlugin("__missing_plugin__"));
    }
}

// ----------------------------------------------------------------------------

TEST_CASE("Construction") {
    lm::log::ScopedInit init;

    SUBCASE("Simple") {
        auto p = lm::comp::create<D>("test::comp::d1", nullptr, { {"v1", 42}, {"v2", 43} });
        REQUIRE(p);
        CHECK(p->f() == 85);
    }

    SUBCASE("Construction (native plugin)") {
        lm::comp::detail::ScopedLoadPlugin pluginGuard("lm_test_plugin");
        REQUIRE(pluginGuard.valid());
        auto p = lm::comp::create<TestPlugin>("testplugin::construct", nullptr, { {"v1", 42}, {"v2", 43} });
        REQUIRE(p);
        CHECK(p->f() == -1);
    }

    SUBCASE("Construction with parent component") {
        auto d = lm::comp::create<D>("test::comp::d1", nullptr, { {"v1", 42}, {"v2", 43} });
        REQUIRE(d);
        auto e = lm::comp::create<E>("test::comp::e1", d.get(), {});
        REQUIRE(e);
        CHECK(e->f() == 86);
    }

    SUBCASE("Construction with underlying component of the parent") {
        auto d = lm::comp::create<D>("test::comp::d1", nullptr, { {"v1", 42}, {"v2", 43} });
        REQUIRE(d);
        auto e1 = lm::comp::create<E>("test::comp::e1", d.get(), {});
        REQUIRE(e1);
        auto e2 = lm::comp::create<E>("test::comp::e2", e1.get(), {});
        REQUIRE(e2);
        CHECK(e2->f() == 87);
    }
}

// ----------------------------------------------------------------------------

TEST_CASE("Serialization of component") {
    lm::log::ScopedInit init;

    SUBCASE("Simple") {
        // Create instance
        const auto p = lm::comp::create<F>("test::comp::f1", nullptr, { {"v", 42} });
        REQUIRE(p);
        CHECK(p->f1() == 43);
        CHECK(p->f2() == 41);
        // Save
        std::stringstream ss;
        p->save(ss);
        // Load
        const auto p2 = lm::comp::create<F>("test::comp::f1", nullptr);
        p2->load(ss);
        CHECK(p2->f1() == 43);
        CHECK(p2->f2() == 41);
    }

    SUBCASE("Serialiation of the instance with references") {
        const auto f1 = lm::comp::create<F>("test::comp::f1", nullptr, { {"v", 42} });
        const auto f2 = lm::comp::create<F>("test::comp::f2", f1.get(), { {"v", 100} });
        CHECK(f2->f1() == 143);
        CHECK(f2->f2() == 141);
        std::stringstream ss;
        f2->save(ss);
        const auto f2_new = lm::comp::create<F>("test::comp::f2", f1.get());
        f2_new->load(ss);
        CHECK(f2_new->f1() == 143);
        CHECK(f2_new->f2() == 141);
    }
}

// ----------------------------------------------------------------------------

TEST_CASE_TEMPLATE("Templated component", T, int, double) {
    lm::log::ScopedInit init;

    SUBCASE("Simple") {
        const auto p = lm::comp::create<G<T>>("test::comp::g1", nullptr);
        REQUIRE(p);
        if constexpr (std::is_same_v<T, int>) {
            CHECK(p->f() == 1);
        }
        if constexpr (std::is_same_v<T, double>) {
            CHECK(p->f() == 2);
        }
    }
    SUBCASE("Plugin") {
        lm::comp::detail::ScopedLoadPlugin pluginGuard("lm_test_plugin");
        REQUIRE(pluginGuard.valid());
        const auto p = lm::comp::create<TestPluginWithTemplate<T>>("testplugin::template", nullptr);
        if constexpr (std::is_same_v<T, int>) {
            CHECK(p->f() == 1);
        }
        if constexpr (std::is_same_v<T, double>) {
            CHECK(p->f() == 2);
        }
    }
}

// ----------------------------------------------------------------------------

LM_NAMESPACE_END(LM_TEST_NAMESPACE)
