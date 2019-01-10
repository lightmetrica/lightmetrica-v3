/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include <lm/component.h>
#include <lm/json.h>
#include <lm/math.h>
#include <iostream>
#include <cereal/cereal.hpp>
#include <cereal/archives/portable_binary.hpp>

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

struct E : public lm::Component {
    virtual int f() = 0;
};

struct E1 final : public E {
    D* d;
    virtual bool construct(const lm::Json& prop) override {
        LM_UNUSED(prop);
        d = parent()->cast<D>();
        return true;
    }
    virtual int f() override {
        return d->f() + 1;
    }
    virtual Component* underlying(const std::string& name) const {
        LM_UNUSED(name);
        return d;
    }
};

struct E2 final : public E {
    D* d;
    virtual bool construct(const lm::Json& prop) override {
        LM_UNUSED(prop);
        d = parent()->underlying()->cast<D>();
        return true;
    }
    virtual int f() override {
        return d->f() + 2;
    }
};

#ifdef LM_TEST_INTERFACE_REG_IMPL
LM_COMP_REG_IMPL(E1, "test::comp::e1");
LM_COMP_REG_IMPL(E2, "test::comp::e2");
#endif

// ----------------------------------------------------------------------------

struct F : public lm::Component {
    virtual int f1() const = 0;
    virtual int f2() const = 0;
};

struct F1 final : public F {
    int v1_;
    int v2_;
    virtual bool construct(const lm::Json& prop) override {
        const int v = prop["v"];
        v1_ = v + 1;
        v2_ = v - 1;
        return true;
    }
    virtual void load(InputArchive& ar) override {
        cereal::PortableBinaryInputArchive ar(stream);
        ar(v1_, v2_);
    }
    virtual void save(OutputArchive& ar) const override {
        cereal::PortableBinaryOutputArchive ar(stream);
        ar(v1_, v2_);
    }
    virtual int f1() const override { return v1_; }
    virtual int f2() const override { return v2_; }
};

struct F2 final : public F {
    int v_;
    F* f_;
    virtual bool construct(const lm::Json& prop) override {
        f_ = parent()->cast<F>();
        v_ = prop["v"];
        return true;
    }
    virtual void load(InputArchive& ar) override {
        cereal::PortableBinaryInputArchive ar(stream);
        ar(v_);
        f_ = parent()->cast<F>();
    }
    virtual void save(OutputArchive& ar) const override {
        cereal::PortableBinaryOutputArchive ar(stream);
        ar(v_);
    }
    virtual int f1() const override { return v_ + f_->f1(); }
    virtual int f2() const override { return v_ + f_->f2(); }
};

#ifdef LM_TEST_INTERFACE_REG_IMPL
LM_COMP_REG_IMPL(F1, "test::comp::f1");
LM_COMP_REG_IMPL(F2, "test::comp::f2");
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
