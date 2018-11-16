/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "lm.h"
#include "pylm_json.h"
#include "pylm_math.h"

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(LM_NAMESPACE)
LM_NAMESPACE_BEGIN(py)

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(detail)
// Allows to access private members to Component instances
class ComponentAccess {
public:
    
};
LM_NAMESPACE_END(detail)

// ----------------------------------------------------------------------------

/*!
    \brief Wraps registration of component instance.
*/
template <typename InterfaceT>
static void regCompWrap(pybind11::object implClass, const char* name) {
    lm::comp::detail::reg(name,
        [implClass = implClass]() -> lm::Component* {
            // Create instance of python class
            auto instPy = implClass();
            // We need to keep track of the python object
            // to manage lifetime of the instance
            auto instCpp = instPy.cast<InterfaceT*>();
            // Assignment to std::any increments ref counter.
            instCpp->ownerRef_ = instPy;
            // Destruction of instPy decreases ref counter.
            return instCpp;
        },
        [](lm::Component* p) -> void {
            // Automatic deref of instPy causes invocation of GC.
            // Note we need one extra deref because one reference is still hold by ownerRef_.
            // The pointer of component is associated with instPy
            // and it will be destructed when it is deallocated by GC.
            auto instPy = std::any_cast<pybind11::object>(p->ownerRef_);
            instPy.dec_ref();
        });
}

/*!
    \brief Wraps creation of component instance.
*/
template <typename InterfaceT>
static pybind11::object createCompWrap(const char* name, Component* parent) {
    auto inst = lm::comp::detail::createComp(name, parent);
    if (!inst) {
        return pybind11::object();
    }
    return castToPythonObject<InterfaceT>(inst);
}

/*!
    \brief Creation of component instance with construction.
*/
template <typename InterfaceT>
static pybind11::object createCompWrap(const char* name, Component* parent, const Json& prop) {
    auto inst = lm::comp::detail::createComp(name, parent);
    if (!inst || !inst->construct(prop)) {
        return pybind11::object();
    }
    return castToPythonObject<InterfaceT>(inst);
}

/*!
    \brief Cast lm::Component to Python object.
*/
template <typename InterfaceT>
static pybind11::object castToPythonObject(Component* inst) {
    // We cannot bind lm::comp::create directly because pybind does not support
    // instantiation of unique_ptr with custom deleter. Internally, pybind requires
    // construction of holder_type (e.g., std::unique_ptr) from T*.
    // Instead we associate lifetime of the object with python object.

    // Extract owner if available
    if (inst->ownerRef_.has_value()) {
        // Returning instantiated pointer incurs another wrapper for the pointer inside pybind.
        // To avoid this, if the instance was created inside python, extract the underlying
        // python object and directly return it.
        auto instPy = std::any_cast<pybind11::object>(inst->ownerRef_);
        return instPy;
    }
    // Otherwise inst is C++ instance so return with standard pointer
    // It should work without registered deleter, underlying unique_ptr will take care of it.
    auto* instT = dynamic_cast<InterfaceT*>(inst);
    return pybind11::cast(instT, pybind11::return_value_policy::take_ownership);
}

/*!
    \brief Adds component-related functions to the binding of a component interface.
*/
#define PYLM_DEF_COMP_BIND(InterfaceT) \
     def_static("reg", &LM_NAMESPACE::py::regCompWrap<InterfaceT>) \
    .def_static("unreg", &LM_NAMESPACE::comp::detail::unreg) \
    .def_static("create", \
        pybind11::overload_cast<const char*, LM_NAMESPACE::Component*>( \
            &LM_NAMESPACE::py::createCompWrap<InterfaceT>)) \
    .def_static("create", \
        pybind11::overload_cast<const char*, LM_NAMESPACE::Component*, const LM_NAMESPACE::Json&>( \
            &LM_NAMESPACE::py::createCompWrap<InterfaceT>))

// Trampoline class for lm::Component
class Component_Py final : public Component {
public:
    virtual bool construct([[maybe_unused]] const Json& prop) override {
        PYBIND11_OVERLOAD_PURE(bool, Component, prop);
    }
};

// ----------------------------------------------------------------------------

/*!
    \brief Binds Lightmetrica to the specified module.
*/
static void init(pybind11::module& m) {
    using namespace pybind11::literals;

    // ------------------------------------------------------------------------

    // component.h
    pybind11::class_<Component, Component_Py>(m, "Component")
        .def(pybind11::init<>())
        .def("construct", &Component::construct)
        .def("parent", &Component::parent)
        .PYLM_DEF_COMP_BIND(Component);

    // ------------------------------------------------------------------------

    // user.h
    m.def("init", &init);
    m.def("shutdown", &shutdown);
    m.def("asset", &asset);
    m.def("primitive", &primitive);
    m.def("primitives", &primitives);
    m.def("render", &render);
    m.def("save", &save);
    m.def("buffer", &buffer);

    // ------------------------------------------------------------------------

    // logger.h
    {
        // Namespaces are handled as submodules
        auto sm = m.def_submodule("log");

        // LogLevel
        pybind11::enum_<log::LogLevel>(sm, "LogLevel")
            .value("Debug", log::LogLevel::Debug)
            .value("Info", log::LogLevel::Info)
            .value("Warn", log::LogLevel::Warn)
            .value("Err", log::LogLevel::Err)
            .value("Progress", log::LogLevel::Progress)
            .value("ProgressEnd", log::LogLevel::ProgressEnd);

        // Log API
        sm.def("init", &log::init);
        sm.def("shutdown", &log::shutdown);
        using LogFuncPtr = void(*)(log::LogLevel, const char*, int, const char*);
        sm.def("log", (LogFuncPtr)&log::log);
        sm.def("updateIndentation", &log::updateIndentation);
    }

    // ------------------------------------------------------------------------

    // Film buffer (film.h)
    pybind11::class_<FilmBuffer>(m, "FilmBuffer", pybind11::buffer_protocol())
        // Register buffer description
        .def_buffer([](FilmBuffer& buf) -> pybind11::buffer_info {
            return pybind11::buffer_info(
                buf.data,
                sizeof(Float),
                pybind11::format_descriptor<Float>::format(),
                3,
                { buf.h, buf.w, 3 },
                { 3 * buf.w * sizeof(Float),
                  3 * sizeof(Float),
                  sizeof(Float) }
            );
        });
}

// ----------------------------------------------------------------------------

LM_NAMESPACE_END(py)
LM_NAMESPACE_END(LM_NAMESPACE)
