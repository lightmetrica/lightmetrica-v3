/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include "test_common.h"
#include <lm/pylm.h>
#include <generated/test_python.h>
#include <pybind11/embed.h>
#include <pybind11/functional.h>
#include <cereal/cereal.hpp>
#include <cereal/archives/portable_binary.hpp>

namespace py = pybind11;
using namespace py::literals;

LM_NAMESPACE_BEGIN(LM_TEST_NAMESPACE)

// ----------------------------------------------------------------------------

struct F : public lm::Component {
    virtual int f1() const = 0;
    virtual int f2() const = 0;
};

struct F1 final : public F {
    int v1_;
    int v2_;
    virtual bool construct(const lm::json& prop, lm::Component* parent) override {
        const int v = prop["v"];
        v1_ = v + 1;
        v2_ = v - 1;
    }
    virtual void load(std::istream& stream, Component* parent) override {
        cereal::PortableBinaryInputArchive ar(stream);
        ar(v1_, v2_);
    }
    virtual void save(std::ostream& stream) const override {
        cereal::PortableBinaryOutputArchive ar(stream);
        ar(v1_, v2_);
    }
    virtual int f1() const override { return v1_; }
    virtual int f2() const override { return v2_; }
};

LM_COMP_REG_IMPL(F1, "test::serial::f1");

// ----------------------------------------------------------------------------

TEST_CASE("Serialization") {
    const auto p = lm::comp::create<F>("test::serial::f1", { {"v", 42} });
    REQUIRE(p);
    CHECK(p->f1() == 43);
    CHECK(p->f2() == 41);
}

TEST_CASE("Serialization (python)") {

}

// ----------------------------------------------------------------------------

LM_NAMESPACE_END(LM_TEST_NAMESPACE)
