/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/detail/component.h>
#include <lm/json.h>
#include <lm/math.h>
#include <iostream>

LM_NAMESPACE_BEGIN(lmtest)

struct TestPlugin : public lm::Component {
    virtual int f() = 0;
};

struct TestPluginWithCtorAndDtor : public lm::Component {
    TestPluginWithCtorAndDtor()  { std::cout << "A"; }
    ~TestPluginWithCtorAndDtor() { std::cout << "~A"; }
};

template <typename T>
struct TestPluginWithTemplate : public lm::Component {
    virtual T f() const = 0;
};

LM_NAMESPACE_END(lmtest)
