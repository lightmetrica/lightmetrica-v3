/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include "test_common.h"
#include <lm/assets.h>

LM_NAMESPACE_BEGIN(LM_TEST_NAMESPACE)

// ----------------------------------------------------------------------------

struct TestAsset : public lm::Component {
    virtual int f() const = 0;
};

struct TestAsset1 final : public TestAsset {
    int v = -1;
    virtual bool construct(const lm::json& prop, lm::Component* parent) override {
        if (prop.count("v")) {
            v = prop["v"];
        }
        return true;
    }
    virtual int f() const override {
        return v;
    }
};

struct TestAsset2 final : public TestAsset {
    int v;
    virtual bool construct(const lm::json& prop, lm::Component* parent) override {
        const auto* other = parent->underlying("asset1")->cast<TestAsset>();
        v = other->f() + 1;
        return true;
    }
    virtual int f() const override {
        return v;
    }
};

LM_COMP_REG_IMPL(TestAsset1, "test::testasset1");
LM_COMP_REG_IMPL(TestAsset2, "test::testasset2");

// ----------------------------------------------------------------------------

TEST_CASE("Assets") {
    lm::log::ScopedInit init;

    auto assets = lm::comp::create<lm::Assets>("assets::default");
    REQUIRE(assets);

    SUBCASE("Load asset without properties") {
        bool result = assets->loadAsset("asset1", "test::testasset1", lm::json());
        CHECK(result);
        auto* a1 = assets->underlying<TestAsset1>("asset1");
        REQUIRE(a1);
        CHECK(a1->f() == -1);
    }

    SUBCASE("Load asset with properties") {
        CHECK(assets->loadAsset("asset1", "test::testasset1", { {"v", 42} }));
        auto* a1 = assets->underlying<TestAsset1>("asset1");
        REQUIRE(a1);
        CHECK(a1->f() == 42);
    }

    SUBCASE("Load asset dependent on an other asset") {
        CHECK(assets->loadAsset("asset1", "test::testasset1", { {"v", 42} }));
        CHECK(assets->loadAsset("asset2", "test::testasset2", {}));
        auto* a2 = assets->underlying<TestAsset2>("asset2");
        REQUIRE(a2);
        CHECK(a2->f() == 43);
    }
}

LM_NAMESPACE_END(LM_TEST_NAMESPACE)
