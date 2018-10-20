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
        return true;
    }
    virtual void load(std::istream& stream, lm::Component* parent) override {
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

struct F2 final : public F {
    int v_;
    F* f_;
    virtual bool construct(const lm::json& prop, lm::Component* parent) override {
        f_ = dynamic_cast<F*>(parent);
        v_ = prop["v"];
        return true;
    }
    virtual void load(std::istream& stream, lm::Component* parent) override {
        cereal::PortableBinaryInputArchive ar(stream);
        ar(v_);
        f_ = dynamic_cast<F*>(parent);
    }
    virtual void save(std::ostream& stream) const override {
        cereal::PortableBinaryOutputArchive ar(stream);
        ar(v_);
    }
    virtual int f1() const override { return v_ + f_->f1(); }
    virtual int f2() const override { return v_ + f_->f2(); }
};

LM_COMP_REG_IMPL(F1, "test::serial::f1");
LM_COMP_REG_IMPL(F2, "test::serial::f2");

// ----------------------------------------------------------------------------

TEST_CASE("Serialization") {
    SUBCASE("Simple") {
        // Create instance
        const auto p = lm::comp::create<F>("test::serial::f1", { {"v", 42} });
        REQUIRE(p);
        CHECK(p->f1() == 43);
        CHECK(p->f2() == 41);
        // Save
        std::stringstream ss;
        p->save(ss);
        // Load
        const auto p2 = lm::comp::create<F>("test::serial::f1");
        p2->load(ss);
        CHECK(p2->f1() == 43);
        CHECK(p2->f2() == 41);
    }

    SUBCASE("Serialiation of the instance with references") {
        const auto f1 = lm::comp::create<F>("test::serial::f1", { {"v", 42} });
        const auto f2 = lm::comp::create<F>("test::serial::f2", { {"v", 100} }, f1.get());
        CHECK(f2->f1() == 143);
        CHECK(f2->f2() == 141);
        std::stringstream ss;
        f2->save(ss);
        const auto f2_new = lm::comp::create<F>("test::serial::f2");
        f2_new->load(ss, f1.get());
        CHECK(f2_new->f1() == 143);
        CHECK(f2_new->f2() == 141);
    }
}

// ----------------------------------------------------------------------------

LM_NAMESPACE_END(LM_TEST_NAMESPACE)
