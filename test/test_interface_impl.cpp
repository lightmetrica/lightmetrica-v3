/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include "test_interface.h"

LM_NAMESPACE_BEGIN(lmtest)

// ----------------------------------------------------------------------------

struct TestPlugin_ final : public TestPlugin {
    virtual int f() override {
        return 42;
    }
};

LM_COMP_REG_IMPL(TestPlugin_, "testplugin::default");

// ----------------------------------------------------------------------------

struct TestPlugin_WithConstruct : public TestPlugin {
    int v1;
    int v2;
    virtual bool construct(const lm::Json& prop) override {
        v1 = prop["v1"].get<int>();
        v2 = prop["v2"].get<int>();
        return true;
    }
    virtual int f() override {
        return v1 - v2;
    }
};

LM_COMP_REG_IMPL(TestPlugin_WithConstruct, "testplugin::construct");

// ----------------------------------------------------------------------------

struct TestPluginWithCtorAndDtor_ final : public TestPluginWithCtorAndDtor {
    TestPluginWithCtorAndDtor_()  { std::cout << "B"; }
    ~TestPluginWithCtorAndDtor_() { std::cout << "~B"; }
};

LM_COMP_REG_IMPL(TestPluginWithCtorAndDtor_, "testpluginxtor::default");

// ----------------------------------------------------------------------------

template <typename T>
struct TestPluginWithTemplate_ final : public TestPluginWithTemplate<T> {
    virtual T f() const override {
        if constexpr (std::is_same_v<T, int>) {
            return 1;
        }
        if constexpr (std::is_same_v<T, double>) {
            return 2;
        }
    }
};

LM_COMP_REG_IMPL(TestPluginWithTemplate_<int>, "testplugin::template");
LM_COMP_REG_IMPL(TestPluginWithTemplate_<double>, "testplugin::template");

// ----------------------------------------------------------------------------

LM_NAMESPACE_END(lmtest)
