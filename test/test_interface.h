/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include <lm/component.h>
#include <lm/json.h>
#include <lm/math.h>
#include <iostream>
#include <lm/ext/cereal/cereal.hpp>
#include <lm/ext/cereal/archives/portable_binary.hpp>

LM_NAMESPACE_BEGIN(lmtest)

// ----------------------------------------------------------------------------

// Some component classes for tests
// Register the classes in the same translation unit

// _begin_snippet: A
struct A : public lm::Component {
    virtual int f1() = 0;
    virtual int f2(int a, int b) = 0;
};

struct A1 final : public A {
    virtual int f1() override { return 42; }
    virtual int f2(int a, int b) override { return a + b; }
};

#ifdef LM_TEST_INTERFACE_REG_IMPL
LM_COMP_REG_IMPL(A1, "test::comp::a1");
#endif
// _end_snippet: A

// ----------------------------------------------------------------------------

struct B : public A {
    virtual int f3() = 0;
};

struct B1 final : public B {
    virtual int f1() override { return 42; }
    virtual int f2(int a, int b) override { return a + b; }
    virtual int f3() override { return 43; }
};

#ifdef LM_TEST_INTERFACE_REG_IMPL
LM_COMP_REG_IMPL(B1, "test::comp::b1");
#endif

// ----------------------------------------------------------------------------

struct C : public lm::Component {
    C() { std::cout << "C"; }
    virtual ~C() { std::cout << "~C"; }
};

struct C1 : public C {
    C1() { std::cout << "C1"; }
    virtual ~C1() { std::cout << "~C1"; }
};

#ifdef LM_TEST_INTERFACE_REG_IMPL
LM_COMP_REG_IMPL(C1, "test::comp::c1");
#endif

// ----------------------------------------------------------------------------

struct D : public lm::Component {
    virtual int f() = 0;
};

struct D1 final : public D {
    int v1;
    int v2;
    virtual bool construct(const lm::Json& prop) override {
        v1 = prop["v1"].get<int>();
        v2 = prop["v2"].get<int>();
        return true;
    }
    virtual int f() override {
        return v1 + v2;
    }
};

#ifdef LM_TEST_INTERFACE_REG_IMPL
LM_COMP_REG_IMPL(D1, "test::comp::d1");
#endif

// ----------------------------------------------------------------------------

template <typename T>
struct G : public lm::Component {
    virtual T f() const = 0;
};

template <typename T>
struct G1 final : public G<T> {
    virtual T f() const override {
        if constexpr (std::is_same_v<T, int>) {
            return 1;
        }
        if constexpr (std::is_same_v<T, double>) {
            return 2;
        }
    }
};

#ifdef LM_TEST_INTERFACE_REG_IMPL
// We can register templated components with the same key
LM_COMP_REG_IMPL(G1<int>, "test::comp::g1");
LM_COMP_REG_IMPL(G1<double>, "test::comp::g1");
#endif

// ----------------------------------------------------------------------------

// Plugins implemented in a different shared library
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

// ----------------------------------------------------------------------------

LM_NAMESPACE_END(lmtest)
