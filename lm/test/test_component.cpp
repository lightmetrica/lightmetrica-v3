/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include "test_common.h"
#include "test_interface.h"
#include <lm/pylm.h>
#include <generated/test_python.h>
#include <pybind11/embed.h>
#include <pybind11/functional.h>
#include <cereal/cereal.hpp>
#include <cereal/archives/portable_binary.hpp>

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

TEST_CASE("Component") {
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
        const auto a = lm::Component::Ptr<A>(b.release(), b.get_deleter());
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
        lm::comp::detail::ScopedLoadPlugin pluginGuard("lm_test_plugin");
        REQUIRE(pluginGuard.valid());
        SUBCASE("Simple") {
            auto p = lm::comp::create<TestPlugin>("testplugin::default");
            REQUIRE(p);
            CHECK(p->f() == 42);
        }
        SUBCASE("Plugin constructor and destructor") {
            const auto out = captureStdout([]() {
                const auto p = lm::comp::create<TestPluginWithCtorAndDtor>("testpluginxtor::default");
                REQUIRE(p);
            });
            CHECK(out == "AB~B~A");
        }
    }

    SUBCASE("Failed to load plugin") {
        REQUIRE(!lm::comp::detail::loadPlugin("__missing_plugin_name__"));
    }
}

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
    virtual bool construct(const lm::json& prop, lm::Component* parent) override {
        PYBIND11_OVERLOAD_PURE(bool, TestPlugin, prop, parent);
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

PYBIND11_EMBEDDED_MODULE(test_comp, m) {
    A_Py::bind(m);
    TestPlugin_Py::bind(m);

    m.def("createA1", []() {
        return dynamic_cast<A*>(lm::comp::detail::createComp("test::comp::a1"));
    });

    m.def("createTestPlugin", []() {
        return dynamic_cast<TestPlugin*>(lm::comp::detail::createComp("testplugin::default"));
    });

    m.def("useA", [](A* a) -> int {
        return a->f1() * 2;
    });
}

// unique_ptr with custom deleter
template <typename T>
using Ptr = std::unique_ptr<T, std::function<void(T*)>>;

// Wraps dereferencing of python object in the deleter
template <typename T, typename... Args>
Ptr<T> create(const char* name, Args... opt) {
    const auto instPy = py::globals()[name](opt...);
    return Ptr<T>(instPy.cast<T*>(), [p = py::reinterpret_borrow<py::object>(instPy)](T*){});
}

TEST_CASE("Python component plugin") {
    Py_SetPythonHome(LM_TEST_PYTHON_ROOT);
    py::scoped_interpreter guard{};
  
    try {
        // Import test module
        py::exec(R"(
            import test_comp
        )", py::globals());

        SUBCASE("Create and use component inside python script") {
            auto locals = py::dict();
            py::exec(R"(
                p = test_comp.createA1()
                r1 = p.f1()
                r2 = p.f2(1, 2)
            )", py::globals(), locals);
            CHECK(locals["r1"].cast<int>() == 42);
            CHECK(locals["r2"].cast<int>() == 3);
        }

        SUBCASE("Use component inside native plugin") {
            lm::comp::detail::ScopedLoadPlugin pluginGuard("lm_test_plugin");
            REQUIRE(pluginGuard.valid());
            auto locals = py::dict();
            py::exec(R"(
                p = test_comp.createTestPlugin()
                r1 = p.f()
            )", py::globals(), locals);
            CHECK(locals["r1"].cast<int>() == 42);
        }

        SUBCASE("Extend component from python") {
            py::exec(R"(
                class A2(test_comp.A):
                    def f1(self):
                        return 43
                    def f2(self, a, b):
                        return a * b

                class A3(test_comp.A):
                    def __init__(self, v):
                        # We need explicit initialization (not super()) of the base class
                        # See https://pybind11.readthedocs.io/en/stable/advanced/classes.html
                        test_comp.A.__init__(self)
                        self.v = v
                    def f1(self):
                        return self.v
                    def f2(self, a, b):
                        return self.v * self.v
            )", py::globals());

            SUBCASE("Instantiate inside python script and use it in python") {
                auto locals = py::dict();
                py::exec(R"(
                    p = A2()
                    r1 = p.f1()
                    r2 = p.f2(2, 3)
                )", py::globals(), locals);
                CHECK(locals["r1"].cast<int>() == 43);
                CHECK(locals["r2"].cast<int>() == 6);
            }

            SUBCASE("Instantiate inside python script and use it in C++") {
                auto locals = py::dict();
                py::exec(R"(
                    p = A2()
                    r1 = test_comp.useA(p)
                )", py::globals(), locals);
                CHECK(locals["r1"].cast<int>() == 86);
            }

            SUBCASE("Instantiate in C++ and use it in C++") {
                // Extract A2
                auto A2 = py::globals()["A2"];
                // Create instance of A2 (python object)
                auto p_py = A2();
                // Cast to A*
                auto p = p_py.cast<A*>();
                // Call function
                const auto result = p->f1();
                CHECK(result == 43);
            }

            SUBCASE("Manage instances in C++") {
                SUBCASE("Without constructor") {
                    auto p = create<A>("A2");
                    REQUIRE(p);
                    CHECK(p->f1() == 43);
                }
                SUBCASE("With constructor") {
                    auto p = create<A>("A3", 1);
                    REQUIRE(p);
                    CHECK(p->f1() == 1);
                }
                SUBCASE("In C++ vector") {
                    std::vector<Ptr<A>> v;
                    for (int i = 0; i < 10; i++) {
                        v.push_back(create<A>("A3", i));
                        REQUIRE(v.back());
                    }
                    for (int i = 0; i < 10; i++) {
                        CHECK(v[i]->f1() == i);
                    }
                }
            }
        }

        SUBCASE("Python component factory") {
            // Define and register component implementation
            py::exec(R"(
                # Implement component A
                class A4(test_comp.A):
                    def f1(self):
                        return 44
                    def f2(self, a, b):
                        return a - b

                # Register
                test_comp.A.reg(A4, 'test::comp::a4')
            )", py::globals());

            SUBCASE("Native embeded plugin") {
                auto locals = py::dict();
                py::exec(R"(
                    p = test_comp.A.create('test::comp::a1')
                    r1 = p.f1()
                    r2 = p.f2(2, 3)
                )", py::globals(), locals);
                CHECK(locals["r1"].cast<int>() == 42);
                CHECK(locals["r2"].cast<int>() == 5);
            }

            SUBCASE("Native external plugin") {
                lm::comp::detail::ScopedLoadPlugin pluginGuard("lm_test_plugin");
                REQUIRE(pluginGuard.valid());
                auto locals = py::dict();
                py::exec(R"(
                    p = test_comp.TestPlugin.create('testplugin::default')
                    r1 = p.f()
                )", py::globals(), locals);
                CHECK(locals["r1"].cast<int>() == 42);
            }

            SUBCASE("Python plugin") {
                auto locals = py::dict();
                py::exec(R"(
                    p = test_comp.A.create('test::comp::a4')
                    r1 = p.f1()
                    r2 = p.f2(2, 3)
                )", py::globals(), locals);
                CHECK(locals["r1"].cast<int>() == 44);
                CHECK(locals["r2"].cast<int>() == -1);
            }

            SUBCASE("Python plugin instantiate from C++") {
                auto p = lm::comp::create<A>("test::comp::a4");
                CHECK(p->f1() == 44);
                CHECK(p->f2(2, 3) == -1);
            }

            py::exec(R"(
                test_comp.A.unreg('test::comp::a4')
            )", py::globals());
        }
    }
    catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        REQUIRE(false);
    }
}

// ----------------------------------------------------------------------------

struct D : public lm::Component {
    virtual int f() = 0;
};

struct D1 final : public D {
    int v1;
    int v2;
    virtual bool construct(const lm::json& prop, lm::Component* parent) override {
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
    virtual bool construct(const lm::json& prop, lm::Component* parent) override {
        d = parent->cast<D>();
        return true;
    }
    virtual int f() override {
        return d->f() + 1;
    }
    virtual Component* underlying(const std::string& name) const {
        return d;
    }
};

struct E2 final : public E {
    D* d;
    virtual bool construct(const lm::json& prop, lm::Component* parent) override {
        d = parent->underlying()->cast<D>();
        return true;
    }
    virtual int f() override {
        return d->f() + 2;
    }
};

LM_COMP_REG_IMPL(E1, "test::comp::e1");
LM_COMP_REG_IMPL(E2, "test::comp::e2");

// ----------------------------------------------------------------------------

TEST_CASE("Construction") {
    SUBCASE("Simple") {
        auto p = lm::comp::create<D>("test::comp::d1", { {"v1", 42}, {"v2", 43} });
        REQUIRE(p);
        CHECK(p->f() == 85);
    }

    SUBCASE("Construction (native plugin)") {
        lm::comp::detail::ScopedLoadPlugin pluginGuard("lm_test_plugin");
        REQUIRE(pluginGuard.valid());
        auto p = lm::comp::create<TestPlugin>("testplugin::construct", { {"v1", 42}, {"v2", 43} });
        REQUIRE(p);
        CHECK(p->f() == -1);
    }

    SUBCASE("Construction with parent component") {
        auto d = lm::comp::create<D>("test::comp::d1", { {"v1", 42}, {"v2", 43} });
        REQUIRE(d);
        auto e = lm::comp::create<E>("test::comp::e1", {}, d.get());
        REQUIRE(e);
        CHECK(e->f() == 86);
    }

    SUBCASE("Construction with underlying component of the parent") {
        auto d = lm::comp::create<D>("test::comp::d1", { {"v1", 42}, {"v2", 43} });
        REQUIRE(d);
        auto e1 = lm::comp::create<E>("test::comp::e1", {}, d.get());
        REQUIRE(e1);
        auto e2 = lm::comp::create<E>("test::comp::e2", {}, e1.get());
        REQUIRE(e2);
        CHECK(e2->f() == 87);
    }
}

// ----------------------------------------------------------------------------

struct D_Py final : public D {
    virtual bool construct(const lm::json& prop, lm::Component* parent) override {
        PYBIND11_OVERLOAD_PURE(bool, D, prop, parent);
    }
    virtual int f() override {
        PYBIND11_OVERLOAD_PURE(int, D, f);
    }
    static void bind(py::module& m) {
        py::class_<D, D_Py, lm::Component>(m, "D")
            .def(py::init<>())
            .def("construct", &D::construct)
            .def("f", &D::f)
            .PYLM_DEF_COMP_BIND(D);
    }
};

struct E_Py final : public E {
    virtual bool construct(const lm::json& prop, lm::Component* parent) override {
        PYBIND11_OVERLOAD_PURE(bool, E, prop, parent);
    }
    virtual int f() override {
        PYBIND11_OVERLOAD_PURE(int, E, f);
    }
    static void bind(py::module& m) {
        py::class_<E, E_Py, lm::Component>(m, "E")
            .def(py::init<>())
            .def("construct", &E::construct)
            .def("f", &E::f)
            .PYLM_DEF_COMP_BIND(E);
    }
};

PYBIND11_EMBEDDED_MODULE(test_comp_2, m) {
    lm::py::bindInterfaces(m);
    D_Py::bind(m);
    E_Py::bind(m);
    TestPlugin_Py::bind(m);
}

TEST_CASE("Construction (python)") {
    Py_SetPythonHome(LM_TEST_PYTHON_ROOT);
    py::scoped_interpreter guard{};

    try {
        py::exec(R"(
            import test_comp_2 as test
        )", py::globals());

        SUBCASE("Simple") {
            auto locals = py::dict();
            py::exec(R"(
                p = test.D.create('test::comp::d1', {'v1':42, 'v2':43}, None)
                r = p.f()
            )", py::globals(), locals);
            CHECK(locals["r"].cast<int>() == 85);
        }

        SUBCASE("Construction (native plugin)") {
            lm::comp::detail::ScopedLoadPlugin pluginGuard("lm_test_plugin");
            REQUIRE(pluginGuard.valid());
            auto locals = py::dict();
            py::exec(R"(
                p = test.TestPlugin.create('testplugin::construct', {'v1':42, 'v2':43}, None)
                r = p.f()
            )", py::globals(), locals);
            CHECK(locals["r"].cast<int>() == -1);
        }

        SUBCASE("Construction with parent component") {
            auto locals = py::dict();
            py::exec(R"(
                d = test.D.create('test::comp::d1', {'v1':42, 'v2':43}, None)
                e = test.E.create('test::comp::e1', None, d)
                r = e.f()
            )", py::globals(), locals);
            CHECK(locals["r"].cast<int>() == 86);
        }

        SUBCASE("Construction with underlying component of the parent") {
            auto locals = py::dict();
            py::exec(R"(
                d  = test.D.create('test::comp::d1', {'v1':42, 'v2':43}, None)
                e1 = test.E.create('test::comp::e1', None, d)
                e2 = test.E.create('test::comp::e2', None, e1)
                r = e2.f()
            )", py::globals(), locals);
            CHECK(locals["r"].cast<int>() == 87);
        }
    }
    catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        REQUIRE(false);
    }
}

// ----------------------------------------------------------------------------

struct F : public lm::Component {
    virtual int f1() const = 0;
    virtual int f2() const = 0;
};

struct F1 final : public F {
    int v1_;
    int v2_;
    virtual bool construct(const lm::json& prop, lm::Component* parent) override {
        const int v = prop["v"];
        v1_ = v + 1;
        v2_ = v - 1;
        return true;
    }
    virtual void load(std::istream& stream, lm::Component* parent) override {
        cereal::PortableBinaryInputArchive ar(stream);
        ar(v1_, v2_);
    }
    virtual void save(std::ostream& stream) const override {
        cereal::PortableBinaryOutputArchive ar(stream);
        ar(v1_, v2_);
    }
    virtual int f1() const override { return v1_; }
    virtual int f2() const override { return v2_; }
};

struct F2 final : public F {
    int v_;
    F* f_;
    virtual bool construct(const lm::json& prop, lm::Component* parent) override {
        f_ = dynamic_cast<F*>(parent);
        v_ = prop["v"];
        return true;
    }
    virtual void load(std::istream& stream, lm::Component* parent) override {
        cereal::PortableBinaryInputArchive ar(stream);
        ar(v_);
        f_ = dynamic_cast<F*>(parent);
    }
    virtual void save(std::ostream& stream) const override {
        cereal::PortableBinaryOutputArchive ar(stream);
        ar(v_);
    }
    virtual int f1() const override { return v_ + f_->f1(); }
    virtual int f2() const override { return v_ + f_->f2(); }
};

LM_COMP_REG_IMPL(F1, "test::comp::f1");
LM_COMP_REG_IMPL(F2, "test::comp::f2");

// ----------------------------------------------------------------------------

TEST_CASE("Serialization") {
    SUBCASE("Simple") {
        // Create instance
        const auto p = lm::comp::create<F>("test::comp::f1", { {"v", 42} });
        REQUIRE(p);
        CHECK(p->f1() == 43);
        CHECK(p->f2() == 41);
        // Save
        std::stringstream ss;
        p->save(ss);
        // Load
        const auto p2 = lm::comp::create<F>("test::comp::f1");
        p2->load(ss);
        CHECK(p2->f1() == 43);
        CHECK(p2->f2() == 41);
    }

    SUBCASE("Serialiation of the instance with references") {
        const auto f1 = lm::comp::create<F>("test::comp::f1", { {"v", 42} });
        const auto f2 = lm::comp::create<F>("test::comp::f2", { {"v", 100} }, f1.get());
        CHECK(f2->f1() == 143);
        CHECK(f2->f2() == 141);
        std::stringstream ss;
        f2->save(ss);
        const auto f2_new = lm::comp::create<F>("test::comp::f2");
        f2_new->load(ss, f1.get());
        CHECK(f2_new->f1() == 143);
        CHECK(f2_new->f2() == 141);
    }
}

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
        LM_UNREACHABLE_RETURN();
    }
};

// We can register templated components with the same key
LM_COMP_REG_IMPL(G1<int>, "test::comp::g1");
LM_COMP_REG_IMPL(G1<double>, "test::comp::g1");

// ----------------------------------------------------------------------------

TEST_CASE_TEMPLATE("Templated component", T, int, double) {
    SUBCASE("Simple") {
        const auto p = lm::comp::create<G<T>>("test::comp::g1");
        REQUIRE(p);
        if constexpr (std::is_same_v<T, int>) {
            CHECK(p->f() == 1);
        }
        if constexpr (std::is_same_v<T, double>) {
            CHECK(p->f() == 2);
        }
    }
    SUBCASE("Plugin") {
        lm::comp::detail::ScopedLoadPlugin pluginGuard("lm_test_plugin");
        REQUIRE(pluginGuard.valid());
        const auto p = lm::comp::create<TestPluginWithTemplate<T>>("testplugin::template");
        if constexpr (std::is_same_v<T, int>) {
            CHECK(p->f() == 1);
        }
        if constexpr (std::is_same_v<T, double>) {
            CHECK(p->f() == 2);
        }
    }
}

// ----------------------------------------------------------------------------

LM_NAMESPACE_END(LM_TEST_NAMESPACE)
