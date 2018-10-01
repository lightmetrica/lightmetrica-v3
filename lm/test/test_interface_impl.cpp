/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include "test_interface.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

struct TestPlugin_ final : public TestPlugin {
    virtual int f() override {
        return 42;
    }
};

LM_COMP_REG_IMPL(TestPlugin_, "testplugin::default");

struct TestPluginWithCtorAndDtor_ final : public TestPluginWithCtorAndDtor {
    TestPluginWithCtorAndDtor_()  { std::cout << "B"; }
    ~TestPluginWithCtorAndDtor_() { std::cout << "~B"; }
};

LM_COMP_REG_IMPL(TestPluginWithCtorAndDtor_, "testpluginxtor::default");

LM_NAMESPACE_END(LM_NAMESPACE)
