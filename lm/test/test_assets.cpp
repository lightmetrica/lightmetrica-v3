/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include "test_common.h"
#include <lm/assets.h>

LM_NAMESPACE_BEGIN(LM_TEST_NAMESPACE)

struct Asset1 : public lm::Component {
    int v = -1;
    virtual bool construct(const lm::json& prop, lm::Component* parent) {
        if (prop) {
            v = prop["v"];
        }
        return true;
    }
};

LM_COMP_REG_IMPL(Asset1, "test::asset::asset_1")

// ----------------------------------------------------------------------------

TEST_CASE("Assets") {
    auto assets = lm::comp::create<lm::Assets>("assets::default");
    REQUIRE(assets);

    SUBCASE("w/o properties") {
        CHECK(assets->loadAsset("asset1", "test::asset::asset_1", {}));
        auto* a1 = assets->underlying<Asset1>("asset1");
        REQUIRE(a1);
        CHECK(a1->v == -1);
    }

    SUBCASE("w/ properties") {
        CHECK(assets->loadAsset("asset1", "test::asset::asset_1", { {"v": 42} }));
        auto* a1 = assets->underlying<Asset1>("asset1");
        REQUIRE(a1);
        CHECK(a1->v == 42);
    }
}

LM_NAMESPACE_END(LM_TEST_NAMESPACE)
