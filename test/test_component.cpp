/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include "test_common.h"
#define LM_TEST_INTERFACE_REG_IMPL
#include "test_interface.h"

LM_NAMESPACE_BEGIN(LM_TEST_NAMESPACE)

// ------------------------------------------------------------------------------------------------

TEST_CASE("Component") {
    lm::log::ScopedInit init_;

    SUBCASE("Simple interface") {
        // _begin_snippet: A_impl
        const auto p = lm::comp::create<A>("test::comp::a1", "");
        REQUIRE(p);
        CHECK(p->f1() == 42);
        CHECK(p->f2(1, 2) == 3);
        // _end_snippet: A_impl
    }

    SUBCASE("Inherited interface") {
        const auto p = lm::comp::create<B>("test::comp::b1", "");
        REQUIRE(p);
        CHECK(p->f1() == 42);
        CHECK(p->f2(1, 2) == 3);
        CHECK(p->f3() == 43);
    }

    SUBCASE("Missing implementation") {
        const auto p = lm::comp::create<A>("test::comp::a_missing", "");
        CHECK(p == nullptr);
    }

    SUBCASE("Cast to parent interface") {
        auto b = lm::comp::create<B>("test::comp::b1", "");
        const auto a = std::unique_ptr<A>(b.release());
        REQUIRE(a);
        CHECK(a->f1() == 42);
        CHECK(a->f2(1, 2) == 3);
    }

    SUBCASE("Constructor and destructor") {
        const auto out = capture_stdout([]() {
            const auto p = lm::comp::create<C>("test::comp::c1", "");
            REQUIRE(p);
        });
        CHECK(out == "CC1~C1~C");
    }

    SUBCASE("Plugin") {
        lm::comp::detail::ScopedLoadPlugin plugin_guard_("lm_test_plugin");
        SUBCASE("Simple") {
            auto p = lm::comp::create<TestPlugin>("testplugin::default", "");
            REQUIRE(p);
            CHECK(p->f() == 42);
        }
        SUBCASE("Plugin constructor and destructor") {
            const auto out = capture_stdout([]() {
                const auto p = lm::comp::create<TestPluginWithCtorAndDtor>("testpluginxtor::default", "");
                REQUIRE(p);
            });
            CHECK(out == "AB~B~A");
        }
    }

    SUBCASE("Failed to load plugin") {
        REQUIRE_THROWS(lm::comp::detail::load_plugin("__missing_plugin__"));
    }
}

// ------------------------------------------------------------------------------------------------

TEST_CASE("Construction") {
    lm::log::ScopedInit init_;

    SUBCASE("Simple") {
        auto p = lm::comp::create<D>("test::comp::d1", "", { {"v1", 42}, {"v2", 43} });
        REQUIRE(p);
        CHECK(p->f() == 85);
    }

    SUBCASE("Construction (native plugin)") {
        lm::comp::detail::ScopedLoadPlugin plugin_guard_("lm_test_plugin");
        auto p = lm::comp::create<TestPlugin>("testplugin::construct", "", { {"v1", 42}, {"v2", 43} });
        REQUIRE(p);
        CHECK(p->f() == -1);
    }
}

// ------------------------------------------------------------------------------------------------

TEST_CASE_TEMPLATE("Templated component", T, int, double) {
    lm::log::ScopedInit init_;

    SUBCASE("Simple") {
        const auto p = lm::comp::create<G<T>>("test::comp::g1", "");
        REQUIRE(p);
        if constexpr (std::is_same_v<T, int>) {
            CHECK(p->f() == 1);
        }
        if constexpr (std::is_same_v<T, double>) {
            CHECK(p->f() == 2);
        }
    }
    SUBCASE("Plugin") {
        lm::comp::detail::ScopedLoadPlugin plugin_guard_("lm_test_plugin");
        const auto p = lm::comp::create<TestPluginWithTemplate<T>>("testplugin::template", "");
        if constexpr (std::is_same_v<T, int>) {
            CHECK(p->f() == 1);
        }
        if constexpr (std::is_same_v<T, double>) {
            CHECK(p->f() == 2);
        }
    }
}

// ------------------------------------------------------------------------------------------------

namespace {

struct H : public lm::Component {
    virtual std::string name() const = 0;
};

struct H_Root_ : public H {
    Ptr<H> p1;

    virtual std::string name() const override {
        return "root";
    }

    virtual void construct(const lm::Json&) override {
        p1 = lm::comp::create<H>("test::comp::h_p1_", make_loc("p1"), {});
    }

    virtual Component* underlying(const std::string& name) const {
        if (name == "p1") {
            return p1.get();
        }
        return nullptr;
    }
};

struct H_P1_ : public H {
    Ptr<H> p2;

    virtual std::string name() const override {
        return "p1";
    }

    virtual void construct(const lm::Json&) override {
        p2 = lm::comp::create<H>("test::comp::h_p2_", make_loc("p2"), {});
    }

    virtual Component* underlying(const std::string& name) const {
        if (name == "p2") {
            return p2.get();
        }
        return nullptr;
    }
};

struct H_P2_ : public H {
    virtual std::string name() const override {
        return "p2";
    }
};

LM_COMP_REG_IMPL(H_Root_, "test::comp::h_root_");
LM_COMP_REG_IMPL(H_P1_, "test::comp::h_p1_");
LM_COMP_REG_IMPL(H_P2_, "test::comp::h_p2_");

}

TEST_CASE("Component query") {
    lm::log::ScopedInit log_;
    
    // Create a component and register it as a root component
    const auto root = lm::comp::create<lm::Component>("test::comp::h_root_", "$", {});
    lm::comp::detail::register_root_comp(root.get());

    // Our hierarchy
    // - $
    //   - p1
    //     - p2

    SUBCASE("get") {
        // Root component
        const auto* root_ = lm::comp::get<H>("$");
        CHECK(root_ == root.get());
        CHECK(root_->name() == "root");

        // p1
        const auto* p1 = lm::comp::get<H>("$.p1");
        CHECK(p1);
        CHECK(p1->name() == "p1");

        // p2
        const auto* p2 = lm::comp::get<H>("$.p1.p2");
        CHECK(p2);
        CHECK(p2->name() == "p2");
    }
}

// ------------------------------------------------------------------------------------------------

LM_NAMESPACE_END(LM_TEST_NAMESPACE)
