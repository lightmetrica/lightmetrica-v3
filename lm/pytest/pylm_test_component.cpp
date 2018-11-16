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
    static void bind(py::module& m) {
        py::class_<A, A_Py>(m, "A")
            .def(py::init<>())
            .def("f1", &A::f1)
            .def("f2", &A::f2)
            .PYLM_DEF_COMP_BIND(A);
    }
};

struct TestPlugin_Py final : public TestPlugin {
    virtual bool construct([[maybe_unused]] const lm::Json& prop) override {
        PYBIND11_OVERLOAD_PURE(bool, TestPlugin, prop);
    }
    virtual int f() override {
        PYBIND11_OVERLOAD_PURE(int, TestPlugin, f);
    }
    static void bind(py::module& m) {
        py::class_<TestPlugin, TestPlugin_Py>(m, "TestPlugin")
            .def(py::init<>())
            .def("construct", &TestPlugin::construct)
            .def("f", &TestPlugin::f)
            .PYLM_DEF_COMP_BIND(TestPlugin);
    }
};

// ----------------------------------------------------------------------------

class PyTestBinder_Component : public PyTestBinder {
public:
    virtual void bind(py::module& m) const {
        lm::py::init(m);
        A_Py::bind(m);
        TestPlugin_Py::bind(m);

        m.def("createA1", []() {
            return dynamic_cast<A*>(lm::comp::detail::createComp("test::comp::a1", nullptr));
        });

        m.def("createTestPlugin", []() {
            return dynamic_cast<TestPlugin*>(lm::comp::detail::createComp("testplugin::default", nullptr));
        });

        m.def("useA", [](A* a) -> int {
            return a->f1() * 2;
        });
    }
};

LM_COMP_REG_IMPL(PyTestBinder_Component, "pytestbinder::component");

LM_NAMESPACE_END(LM_TEST_NAMESPACE)
