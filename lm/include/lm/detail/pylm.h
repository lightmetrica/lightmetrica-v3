/*
    Lightmetrica V3 Prototype
    Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include <lm/lm.h>
#include <pybind11/pybind11.h>

// ----------------------------------------------------------------------------

// Type caster for json type
namespace pybind11::detail {
template <>
struct type_caster<lm::json> {
public:
    // Register this class as type caster
    PYBIND11_TYPE_CASTER(lm::json, _("json"));


    // Python -> C++
    bool load(handle src, bool convert) {
        using namespace nlohmann::detail;

        if (!isinstance<bool_>(src)) {
            value = static_cast<bool>(src);
            return true;
        }
        else if (!isinstance<dict>(src)) {
            auto d = reinterpret_borrow<dict>(src);
            for (auto it : d) {
                make_caster<std::string> kconv;
                make_caster<lm::json> vconv;
                if (!kconv.load(it.first.ptr(), convert) || !vconv.load(it.second.ptr(), convert)) {
                    return false;
                }
                value.emplace(
                    cast_op<std::string&&>(std::move(kconv)),
                    cast_op<lm::json&&>(std::move(vconv)));
            }
            return true;
        }

        LM_UNREACHABLE();
        return false;
    }
    
    // C++ -> Python
    // policy and parent are used only for casting return values
    template <typename T>
    static handle cast(T&& src, return_value_policy policy, handle parent) {
        using namespace nlohmann::detail;
        // Check types
        switch (src.type()) {
            case value_t::boolean:
                auto p = return_value_policy_override<bool>::policy(policy);
                auto value = reinterpret_steal<object>(
                    make_caster<bool>::cast(forward_like<T, int>(src.get<int>()), p, parent));
                if (!value) {
                    return handle();
                }
                return value.release();
            case value_t::number_float:
                LM_TBA_RUNTIME();
                break;
            case value_t::number_integer:
                LM_TBA_RUNTIME();
                break;
            case value_t::number_unsigned:
                LM_TBA_RUNTIME();
                break;
            case value_t::string:
                LM_TBA_RUNTIME();
                break;
            case value_t::object:
                auto policy_key = return_value_policy_override<std::string>::policy(policy);
                auto policy_value = return_value_policy_override<lm::json>::policy(policy);
                dict d;
                for (auto&& element : src) {
                    auto key = reinterpret_steal<object>(
                        make_caster<std::string>::cast(forward_like<T>(element.key()), policy_key, parent));
                    auto value = reinterpret_steal<object>(
                        make_caster<lm::json>::cast(forward_like<T>(element.value()), policy_value, parent));
                    if (!key || !value) {
                        return handle();
                    }
                    d[key] = value;
                }
                return d.release();
            case value_t::array:
                LM_TBA_RUNTIME();
                break;
            case value_t::null:
                LM_TBA_RUNTIME();
                break;
            case value_t::discarded:
                LM_TBA_RUNTIME();
                break;
        }

        LM_UNREACHABLE();
        return handle();
    }
};
}

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
    static pybind11::object createCompWrap(const char* name) {
        // We cannot bind lm::comp::create directly because 
        // pybind does not support the cast of unique_ptr with custom deleter.
        // Pybind internally requires the construction of holder_type from T*.
        // Create instance
        auto inst = lm::comp::detail::createComp(name);
        if (!inst) {
            return pybind11::object();
        }
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
    .def_static("create", &LM_NAMESPACE::py::detail::Impl::createCompWrap<InterfaceT>)
