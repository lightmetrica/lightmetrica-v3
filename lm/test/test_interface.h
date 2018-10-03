/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/detail/comp.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

struct TestPlugin : public Component {
    virtual int f() = 0;
};

struct TestPluginWithCtorAndDtor : public Component {
    TestPluginWithCtorAndDtor()  { std::cout << "A"; }
    ~TestPluginWithCtorAndDtor() { std::cout << "~A"; }
};

LM_NAMESPACE_END(LM_NAMESPACE)
