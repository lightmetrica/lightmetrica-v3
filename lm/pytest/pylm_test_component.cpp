/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#define LM_TEST_INTERFACE_REG_IMPL
#include <test_interface.h>
#include "pylm_test.h"

namespace py = pybind11;
using namespace py::literals;

LM_NAMESPACE_BEGIN(LM_TEST_NAMESPACE)

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

        m.def("createA4AndCallFuncs", []() -> std::tuple<int, int> {
            // test::comp::a4 should be defined inside Python script
            int v1, v2;
            {
                auto p = lm::comp::create<A>("test::comp::a4", nullptr);
                v1 = p->f1();
                v2 = p->f2(2, 3);
            }
            return { v1, v2 };
        });
    }
};

LM_COMP_REG_IMPL(PyTestBinder_Component, "pytestbinder::component");

LM_NAMESPACE_END(LM_TEST_NAMESPACE)
