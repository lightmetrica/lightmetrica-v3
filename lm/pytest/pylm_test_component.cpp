/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <test_interface.h>
#include "pylm_test.h"

namespace py = pybind11;
using namespace py::literals;

LM_NAMESPACE_BEGIN(LM_TEST_NAMESPACE)

// ----------------------------------------------------------------------------

struct A : public lm::Component {
    virtual int f1() = 0;
    virtual int f2(int a, int b) = 0;
};

struct A1 final : public A {
    virtual int f1() override { return 42; }
    virtual int f2(int a, int b) override { return a + b; }
};

LM_COMP_REG_IMPL(A1, "test::comp::a1");

// ----------------------------------------------------------------------------

// Define a trampoline class (see ref) for the interface A
// https://pybind11.readthedocs.io/en/stable/advanced/classes.html
struct A_Py final : public A {
    virtual int f1() override {
        PYBIND11_OVERLOAD_PURE(int, A, f1);
    }
    virtual int f2(int a, int b) override {
        PYBIND11_OVERLOAD_PURE(int, A, f2, a, b);
    }
};

struct TestPlugin_Py final : public TestPlugin {
    virtual bool construct([[maybe_unused]] const lm::Json& prop) override {
        PYBIND11_OVERLOAD_PURE(bool, TestPlugin, prop);
    }
    virtual int f() override {
        PYBIND11_OVERLOAD_PURE(int, TestPlugin, f);
    }
};

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

LM_COMP_REG_IMPL(D1, "test::comp::d1");

// ----------------------------------------------------------------------------

struct E : public lm::Component {
    virtual int f() = 0;
};

struct E1 final : public E {
    D* d;
    virtual bool construct(const lm::Json& prop) override {
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

LM_COMP_REG_IMPL(E1, "test::comp::e1");
LM_COMP_REG_IMPL(E2, "test::comp::e2");

// ----------------------------------------------------------------------------

struct D_Py final : public D {
    virtual bool construct([[maybe_unused]] const lm::Json& prop) override {
        PYBIND11_OVERLOAD_PURE(bool, D, prop);
    }
    virtual int f() override {
        PYBIND11_OVERLOAD_PURE(int, D, f);
    }
};

struct E_Py final : public E {
    virtual bool construct([[maybe_unused]] const lm::Json& prop) override {
        PYBIND11_OVERLOAD_PURE(bool, E, prop);
    }
    virtual int f() override {
        PYBIND11_OVERLOAD_PURE(int, E, f);
    }
};

// ----------------------------------------------------------------------------

class PyTestBinder_Component : public PyTestBinder {
public:
    virtual void bind(py::module& m) const {
        py::class_<A, A_Py, Ptr<A>>(m, "A")
            .def(py::init<>())
            .def("f1", &A::f1)
            .def("f2", &A::f2)
            .PYLM_DEF_COMP_BIND(A);

        py::class_<TestPlugin, TestPlugin_Py, Ptr<TestPlugin>>(m, "TestPlugin")
            .def(py::init<>())
            .def("construct", &TestPlugin::construct)
            .def("f", &TestPlugin::f)
            .PYLM_DEF_COMP_BIND(TestPlugin);

        py::class_<D, D_Py, lm::Component, Ptr<D>>(m, "D")
            .def(py::init<>())
            .def("construct", &D::construct)
            .def("f", &D::f)
            .PYLM_DEF_COMP_BIND(D);

        py::class_<E, E_Py, lm::Component, Ptr<E>>(m, "E")
            .def(py::init<>())
            .def("construct", &E::construct)
            .def("f", &E::f)
            .PYLM_DEF_COMP_BIND(E);

        // --------------------------------------------------------------------

        m.def("createA1", []() {
            return dynamic_cast<A*>(
                lm::comp::detail::createComp("test::comp::a1", nullptr));
        });

        m.def("createTestPlugin", []() {
            // Use .release() as pybind11 does not support direct conversion of Ptr<T> types
            return lm::comp::create<TestPlugin>("testplugin::default", nullptr).release();
        });

        m.def("useA", [](A* a) -> int {
            return a->f1() * 2;
        });
    }
};

LM_COMP_REG_IMPL(PyTestBinder_Component, "pytestbinder::component");

LM_NAMESPACE_END(LM_TEST_NAMESPACE)
