/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#define LM_TEST_INTERFACE_REG_IMPL
#include <test_interface.h>
#include "pylm_test.h"

namespace py = pybind11;
using namespace py::literals;

LM_NAMESPACE_BEGIN(LM_TEST_NAMESPACE)

class PyTestBinder_Component : public PyTestBinder {
public:
    virtual void bind(py::module& m) const {
        // Define a trampoline class (see ref) for the interface A
        // https://pybind11.readthedocs.io/en/stable/advanced/classes.html
        struct A_Py final : public A {
            virtual bool construct(const lm::Json& prop) override {
                PYBIND11_OVERLOAD(bool, A, construct, prop);
            }
            virtual int f1() override {
                PYBIND11_OVERLOAD_PURE(int, A, f1);
            }
            virtual int f2(int a, int b) override {
                PYBIND11_OVERLOAD_PURE(int, A, f2, a, b);
            }
        };
        // You must not add .def() for construct() function
        // which is already registered in parent class.
        py::class_<A, A_Py, Ptr<A>>(m, "A")
            .def(py::init<>())
            .def("f1", &A::f1)
            .def("f2", &A::f2)
            .PYLM_DEF_COMP_BIND(A);

        // --------------------------------------------------------------------

        struct TestPlugin_Py final : public TestPlugin {
            virtual bool construct(const lm::Json& prop) override {
                PYBIND11_OVERLOAD(bool, TestPlugin, construct, prop);
            }
            virtual int f() override {
                PYBIND11_OVERLOAD_PURE(int, TestPlugin, f);
            }
        };
        py::class_<TestPlugin, TestPlugin_Py, Ptr<TestPlugin>>(m, "TestPlugin")
            .def(py::init<>())
            .def("f", &TestPlugin::f)
            .PYLM_DEF_COMP_BIND(TestPlugin);

        // --------------------------------------------------------------------

        struct D_Py final : public D {
            virtual bool construct(const lm::Json& prop) override {
                PYBIND11_OVERLOAD(bool, D, construct, prop);
            }
            virtual int f() override {
                PYBIND11_OVERLOAD_PURE(int, D, f);
            }
        };
        py::class_<D, D_Py, lm::Component, Ptr<D>>(m, "D")
            .def(py::init<>())
            .def("f", &D::f)
            .PYLM_DEF_COMP_BIND(D);

        // --------------------------------------------------------------------

#if 0
        struct E_Py final : public E {
            virtual bool construct(const lm::Json& prop) override {
                PYBIND11_OVERLOAD(bool, E, construct, prop);
            }
            virtual int f() override {
                PYBIND11_OVERLOAD_PURE(int, E, f);
            }
        };
        py::class_<E, E_Py, lm::Component, Ptr<E>>(m, "E")
            .def(py::init<>())
            .def("f", &E::f)
            .PYLM_DEF_COMP_BIND(E);
#endif

        // --------------------------------------------------------------------

        m.def("createA1", []() {
            return dynamic_cast<A*>(
                lm::comp::detail::createComp("test::comp::a1"));
        });

        m.def("createTestPlugin", []() {
            // Use .release() as pybind11 does not support direct conversion of Ptr<T> types
            return lm::comp::create<TestPlugin>("testplugin::default").release();
        });

        m.def("useA", [](A* a) -> int {
            return a->f1() * 2;
        });

        m.def("createA4AndCallFuncs", []() -> std::tuple<int, int> {
            // test::comp::a4 should be defined inside Python script
            int v1, v2;
            {
                auto p = lm::comp::create<A>("test::comp::a4");
                v1 = p->f1();
                v2 = p->f2(2, 3);
            }
            return { v1, v2 };
        });

        m.def("createA5AndCallFuncs", []() -> std::tuple<int, int> {
            int v1, v2;
            {
                auto p = lm::comp::create<A>("test::comp::a5", {{"v", 7}});
                v1 = p->f1();
                v2 = p->f2(1, 2);
            }
            return { v1, v2 };
        });
    }
};

LM_COMP_REG_IMPL(PyTestBinder_Component, "pytestbinder::component");

LM_NAMESPACE_END(LM_TEST_NAMESPACE)
