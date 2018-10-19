/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include "test_common.h"
#include <generated/test_python.h>
#include <pybind11/embed.h>
#include <pybind11/functional.h>
#include <lm/pylm.h>
#include <cereal/cereal.hpp>

namespace py = pybind11;
using namespace py::literals;

LM_NAMESPACE_BEGIN(LM_TEST_NAMESPACE)

// ----------------------------------------------------------------------------

struct F : public lm::Component {
    virtual int f() = 0;
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

    }
    virtual void save(std::ostream& stream) const override {

    }
};

// ----------------------------------------------------------------------------

TEST_CASE("Serialization") {

}

TEST_CASE("Serialization (python)") {

}

// ----------------------------------------------------------------------------

LM_NAMESPACE_END(LM_TEST_NAMESPACE)
