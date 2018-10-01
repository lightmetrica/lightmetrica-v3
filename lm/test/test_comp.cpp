/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include "test_common.h"
#include "test_interface.h"
#include <generated/test_python.h>
#include <pybind11/embed.h>
#include <pybind11/functional.h>

namespace py = pybind11;
using namespace py::literals;

LM_NAMESPACE_BEGIN(LM_TEST_NAMESPACE)

// ----------------------------------------------------------------------------

// _begin_snippet: A
struct A : public lm::Component {
    virtual int f1() = 0;
    virtual int f2(int a, int b) = 0;
};

struct A1 final : public A {
    virtual int f1() override { return 42; }
    virtual int f2(int a, int b) override { return a + b; }
};

LM_COMP_REG_IMPL(A1, "test::comp::a1");
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

LM_COMP_REG_IMPL(B1, "test::comp::b1");

// ----------------------------------------------------------------------------

struct C : public lm::Component {
    C() { std::cout << "C"; }
    virtual ~C() { std::cout << "~C"; }
};

struct C1 : public C {
    C1() { std::cout << "C1"; }
    virtual ~C1() { std::cout << "~C1"; }
};

LM_COMP_REG_IMPL(C1, "test::comp::c1");

// ----------------------------------------------------------------------------

TEST_CASE("Component")
{
    SUBCASE("Simple interface") {
        // _begin_snippet: A_impl
        const auto p = lm::comp::create<A>("test::comp::a1");
        REQUIRE(p);
        CHECK(p->f1() == 42);
        CHECK(p->f2(1, 2) == 3);
        // _end_snippet: A_impl
    }

    SUBCASE("Inherited interface") {
        const auto p = lm::comp::create<B>("test::comp::b1");
        REQUIRE(p);
        CHECK(p->f1() == 42);
        CHECK(p->f2(1, 2) == 3);
        CHECK(p->f3() == 43);
    }

    SUBCASE("Missing implementation") {
        const auto p = lm::comp::create<A>("test::comp::a_missing");
        CHECK(p == nullptr);
    }

    SUBCASE("Cast to parent interface") {
        auto b = lm::comp::create<B>("test::comp::b1");
        const auto a = lm::comp::UniquePtr<A>(std::move(b));
        REQUIRE(a);
        CHECK(a->f1() == 42);
        CHECK(a->f2(1, 2) == 3);
    }

    SUBCASE("Constructor and destructor") {
        const auto out = captureStdout([]() {
            const auto p = lm::comp::create<C>("test::comp::c1");
            REQUIRE(p);
        });
        CHECK(out == "CC1~C1~C");
    }

    SUBCASE("Plugin") {
        REQUIRE(lm::comp::detail::loadPlugin("lm_test_plugin"));
        SUBCASE("Simple") {
            auto p = lm::comp::create<lm::TestPlugin>("testplugin::default");
            REQUIRE(p);
            CHECK(p->f() == 42);
        }
        SUBCASE("Plugin constructor and destructor") {
            const auto out = captureStdout([]() {
                const auto p = lm::comp::create<lm::TestPluginWithCtorAndDtor>("testpluginxtor::default");
                REQUIRE(p);
            });
            CHECK(out == "AB~B~A");
        }
        lm::comp::detail::unloadPlugins();
    }

    SUBCASE("Failed to load plugin") {
        REQUIRE(!lm::comp::detail::loadPlugin("__missing_plugin_name__"));
    }
}

// ----------------------------------------------------------------------------

struct Component_Py : public lm::Component {
    Component_Py() = default;
};

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

namespace {
    void deleterA(A* p) {
        LM_SAFE_DELETE(p);
    }
}


template <typename T>
using UniquePtr = std::unique_ptr<T, void(*)(void*)>;

PYBIND11_EMBEDDED_MODULE(test_comp, m) {
    py::class_<lm::Component, Component_Py>(m, "Component")
        .def(py::init<>());

    py::class_<A, A_Py>(m, "A")
        .def(py::init<>())
        .def("f1", &A::f1)
        .def("f2", &A::f2);

    // Bind a factory function that returns a unique_ptr<A>
    //m.def("createA1", []() {
    //    // Use decltype(deleter) as a type of the deleter. Otherwise runtime error occurs.
    //    //const auto deleter = [](A* p) { LM_SAFE_DELETE(p); };
    //    return std::unique_ptr<A, decltype(&deleterA)>(new A1, &deleterA);
    //});

    m.def("createA1", []() {
        const auto deleter = [](A* p) {
            LM_SAFE_DELETE(p);
        };
        return std::unique_ptr<A, decltype(deleter)>(new A1, deleter);
        //return UniquePtr<A>(new A1, deleter);
    });

    //m.def("createA1", []() {
    //    return lm_unique_ptr<A>(new A1, [](A* p) { LM_SAFE_DELETE(p); });
    //});

    //m.def("createTestPlugin", []() {
    //    const auto deleter = [](TestPlugin*) {
    //        LM_SAFE_DELETE(p);
    //    }
    //}):
}

TEST_CASE("Python component plugin")
{
    Py_SetPythonHome(LM_TEST_PYTHON_ROOT);
    py::scoped_interpreter guard{};
  
    try {
        SUBCASE("Create and use component inside python script") {
            auto locals = py::dict();
            py::exec(R"(
                import test_comp
                p = test_comp.createA1()
                r1 = p.f1()
                r2 = p.f2(1, 2)
            )", py::globals(), locals);
            CHECK(locals["r1"].cast<int>() == 42);
            CHECK(locals["r2"].cast<int>() == 3);
        }
    }
    catch (const std::runtime_error& e) {
        std::cout << e.what() << std::endl;
    }

    //SUBCASE("Use component inside plugin") {
    //    auto locals = py::dict();
    //    py::exec(R"(
    //        
    //    )", py::globals(), locals);
    //}
}

// ----------------------------------------------------------------------------

LM_NAMESPACE_END(LM_TEST_NAMESPACE)
