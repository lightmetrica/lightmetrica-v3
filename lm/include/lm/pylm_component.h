/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "component.h"
#include <pybind11/pybind11.h>

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(LM_NAMESPACE)
LM_NAMESPACE_BEGIN(py)
LM_NAMESPACE_BEGIN(detail)

// ----------------------------------------------------------------------------

class Impl {
public:
    LM_DISABLE_CONSTRUCT(Impl);

public:
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

private:
    template <typename InterfaceT>
    static pybind11::object castToPythonObject(lm::Component* inst) {
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

};

// ----------------------------------------------------------------------------

LM_NAMESPACE_END(detail)
LM_NAMESPACE_END(py)
LM_NAMESPACE_END(LM_NAMESPACE)

// ----------------------------------------------------------------------------

/*!
    \brief Adds component-related functions to the binding of a component interface.
*/
#define PYLM_DEF_COMP_BIND(InterfaceT) \
     def_static("reg", &LM_NAMESPACE::py::detail::Impl::regCompWrap<InterfaceT>) \
    .def_static("unreg", &LM_NAMESPACE::comp::detail::unreg) \
    .def_static("create", \
        pybind11::overload_cast<const char*, LM_NAMESPACE::Component*>( \
            &LM_NAMESPACE::py::detail::Impl::createCompWrap<InterfaceT>)) \
    .def_static("create", \
        pybind11::overload_cast<const char*, LM_NAMESPACE::Component*, const LM_NAMESPACE::Json&>( \
            &LM_NAMESPACE::py::detail::Impl::createCompWrap<InterfaceT>))

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(LM_NAMESPACE)
LM_NAMESPACE_BEGIN(py)
LM_NAMESPACE_BEGIN(detail)

// Trampoline class for lm::Component
class Component_Py final : public Component {
public:
    virtual bool construct([[maybe_unused]] const Json& prop) override {
        PYBIND11_OVERLOAD_PURE(bool, Component, prop);
    }
    static void bind(pybind11::module& m) {
        pybind11::class_<Component, Component_Py>(m, "Component")
            .def(pybind11::init<>())
            .def("construct", &Component::construct)
            .def("parent", &Component::parent)
            .PYLM_DEF_COMP_BIND(Component);
    }
};

LM_NAMESPACE_END(detail)
LM_NAMESPACE_END(py)
LM_NAMESPACE_END(LM_NAMESPACE)
