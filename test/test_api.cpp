/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include "test_common.h"
#include <lm/user.h>
#include <lm/scene.h>

LM_NAMESPACE_BEGIN(LM_TEST_NAMESPACE)

// ----------------------------------------------------------------------------

//// Stub mesh
//struct StubMesh : public lm::Component {
//    int v_ = 0;
//    virtual bool construct(const lmJsonon& prop, lm::Component* parent) {
//        v_ = prop["v"];
//    }
//};
//
//LM_COMP_REG_IMPL(StubMesh, "apitest::mesh::stub");
//
//// ----------------------------------------------------------------------------
//
//TEST_CASE("User API") {
//    SUBCASE("Load stub asset") {
//        lm::detail::ScopedInit init;
//
//        // Add assets
//        lm::asset("m1", "apitest::mesh::stub", { {"v", 42} });
//
//        // Add primitives
//        lm::primitive("p1", lm::mat4(1), {
//            { "mesh", "m1" }
//        });
//
//        // Get the created primitive
//        const auto* p = lm::primitive("p1");
//        REQUIRE(p);
//        CHECK(p->cast<StubMesh>()->v_ == 42);
//    }
//
//    SUBCASE("Load OBJ model") {
//        lm::detail::ScopedInit init;
//
//        // Load OBJ model
//        
//    }
//}

//TEST_CASE("primitive")
//{
////! [primitive example]
//    const int id = lm::api::primitive("test");
//    CHECK(id == 42);
////! [primitive example]
//}

LM_NAMESPACE_END(LM_TEST_NAMESPACE)
