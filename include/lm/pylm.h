/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include <lm/core.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <pybind11/numpy.h>

// ------------------------------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(pybind11::detail)

// Type caster for json type
template <>
struct type_caster<lm::Json> {
public:
    // Register this class as type caster
    PYBIND11_TYPE_CASTER(lm::Json, _("json"));

    // Python -> C++
    bool load(handle src, bool convert) {
        if (!convert) {
            // No-convert mode
            return false;
        }

        using namespace nlohmann::detail;
        if (isinstance<none>(src)) {
            value = {};
        }
        else if (isinstance<bool_>(src)) {
            value = src.cast<bool>();
        }
        else if (isinstance<float_>(src)) {
            value = src.cast<double>();
        }
        else if (isinstance<int_>(src)) {
            value = src.cast<int>();
        }
        else if (isinstance<str>(src)) {
            value = src.cast<std::string>();
        }
        else if (isinstance<dict>(src)) {
            auto d = reinterpret_borrow<dict>(src);
            value = lm::Json(value_t::object);
            for (auto it : d) {
                make_caster<std::string> kconv;
                make_caster<lm::Json> vconv;
                if (!kconv.load(it.first.ptr(), convert) || !vconv.load(it.second.ptr(), convert)) {
                    return false;
                }
                value.emplace(
                    cast_op<std::string&&>(std::move(kconv)),
                    cast_op<lm::Json&&>(std::move(vconv)));
            }
        }
        else if (isinstance<sequence>(src)) {
            auto s = reinterpret_borrow<sequence>(src);
            value = lm::Json(value_t::array);
            for (auto it : s) {
                make_caster<lm::Json> vconv;
                if (!vconv.load(it, convert)) {
                    return false;
                }
                value.push_back(cast_op<lm::Json&&>(std::move(vconv)));
            }
        }
        else {
            // Extract underlying C++ type
            // Check if the underlying type is registered to pybind,
            // and the number of base class is one assuming multiple inheritance is not used.
            auto& bases = all_type_info((PyTypeObject*)src.get_type().ptr());
            if (bases.size() == 1) {
                void* ptr = values_and_holders(reinterpret_cast<instance*>(src.ptr())).begin()->value_ptr();
                auto* comp = static_cast<lm::Component*>(ptr);
                // Extranct the locator of the instance
                value = comp->loc();
                return true;
            }

            LM_UNREACHABLE();
            return false;
        }
        return true;
    }
    
    // C++ -> Python
    // policy and parent are used only for casting return values
    static handle cast(const lm::Json& src, return_value_policy policy, handle parent) {
        using namespace nlohmann::detail;
        // Check types
        switch (src.type()) {
            case value_t::null: {
                return none().release();
            }
            case value_t::boolean: {
                return cast_to_python_object<bool>(src, policy, std::move(parent));
            }
            case value_t::number_float: {
                return cast_to_python_object<double>(src, policy, std::move(parent));
            }
            case value_t::number_integer: { [[fallthrough]]; }
            case value_t::number_unsigned: {
                return cast_to_python_object<int>(src, policy, std::move(parent));
            }
            case value_t::string: {
                return cast_to_python_object<std::string>(src, policy, std::move(parent));
            }
            case value_t::object: {
                auto policy_key = return_value_policy_override<std::string>::policy(policy);
                auto policy_value = return_value_policy_override<lm::Json>::policy(policy);
                dict d;
                for (lm::Json::const_iterator it = src.begin(); it != src.end(); ++it) {
                    auto k = reinterpret_steal<object>(
                        make_caster<std::string>::cast(it.key(), policy_key, parent));
                    auto v = reinterpret_steal<object>(
                        make_caster<lm::Json>::cast(it.value(), policy_value, parent));
                    if (!k || !v) {
                        return handle();
                    }
                    d[k] = v;
                }
                return d.release();
            }
            case value_t::array: {
                auto policy_value = return_value_policy_override<lm::Json>::policy(policy);
                list l(src.size());
                size_t index = 0;
                for (auto&& element : src) {
                    auto v = reinterpret_steal<object>(
                        make_caster<lm::Json>::cast(element, policy_value, parent));
                    if (!v) {
                        return handle();
                    }
                    PyList_SET_ITEM(l.ptr(), (ssize_t)(index++), v.release().ptr());
                }
                return l.release();
            }
            case value_t::discarded: {
                LM_UNREACHABLE();
                break;
            }
        }
        LM_UNREACHABLE();
        return handle();
    }

private:
    template <typename U>
    static handle cast_to_python_object(const lm::Json& src, return_value_policy policy, handle&& parent) {
        auto p = return_value_policy_override<U>::policy(policy);
        auto v = reinterpret_steal<object>(make_caster<U>::cast(src.get<U>(), p, parent));
        if (!v) {
            return handle();
        }
        return v.release();
    }

};

// Type caster for glm::vec type
template <int N, typename T, glm::qualifier Q>
struct type_caster<glm::vec<N, T, Q>> {
    using VecT = glm::vec<N, T, Q>;

    // Register this class as type caster
    PYBIND11_TYPE_CASTER(VecT, _("glm::vec"));

    // Python -> C++
    bool load(handle src, bool convert) {
        if (!convert) {
            // No-convert mode
            return false;
        }

        if (!isinstance<array_t<T>>(src)) {
            // Invalid numpy array type
            return false;
        }

        // Ensure that the argument is a numpy array of correct type
        auto buf = array::ensure(src);
        if (!buf) {
            return false;
        }

        // Check dimension
        if (buf.ndim() != 1) {
            return false;
        }

        // Copy values
        memcpy(&value.data, buf.data(), N * sizeof(T));

        return true;
    }

    // C++ -> Python
    static handle cast(const VecT& src, return_value_policy policy, handle parent) {
        LM_UNUSED(policy, parent);

        // Always copy the value irrespective to the return value policy
        auto* src_copy = new VecT(src);
        capsule base(src_copy, [](void* o) {
            delete static_cast<VecT*>(o);
        });

        // Create numpy array from VecT
        array a(
            {N},          // Shapes
            {sizeof(T)},  // Strides
            (T*)src_copy, // Data
            base          // Parent handle
        );
        return a.release();
    }
};

// Type caster for glm::mat type
template <int C, int R, typename T, glm::qualifier Q>
struct type_caster<glm::mat<C, R, T, Q>> {
    using MatT = glm::mat<C, R, T, Q>;

    // Register this class as type caster
    PYBIND11_TYPE_CASTER(MatT, _("glm::mat"));

    // Python -> C++
    bool load(handle src, bool convert) {
        if (!convert) {
            // No-convert mode
            return false;
        }

        if (!isinstance<array_t<T>>(src)) {
            // Invalid numpy array type
            return false;
        }

        // Ensure that the argument is a numpy array of correct type
        auto buf = array::ensure(src);
        if (!buf) {
            return false;
        }

        // Check dimension
        if (buf.ndim() != 2) {
            return false;
        }

        // Copy values
        memcpy(&value[0].data, buf.data(), C * R * sizeof(T));

        // Transpose the copied values because
        // glm is column major on the other hand numpy is row major.
        value = glm::transpose(value);

        return true;
    }

    // C++ -> Python
    static handle cast(const MatT& src, return_value_policy policy, handle parent) {
        LM_UNUSED(policy, parent);

        // Always copy the value irrespective to the return value policy
        auto* src_copy = new MatT(glm::transpose(src));
        capsule base(src_copy, [](void* o) {
            delete static_cast<MatT*>(o);
        });

        // Create numpy array from MatT
        array a(
            {R, C},                    // Shapes
            {R*sizeof(T), sizeof(T)},  // Strides
            (T*)src_copy,              // Data
            base                       // Parent handle
        );
        return a.release();
    }
};

LM_NAMESPACE_END(pybind11::detail)

// ------------------------------------------------------------------------------------------------

// Add component's unique_ptr as internal holder type
// Third argument must be false (as for default)
// otherwise a new holder instance is always created when we cast
// a return type even if we specify return_value_policy::reference.
PYBIND11_DECLARE_HOLDER_TYPE(T, lm::Component::Ptr<T>, false);

// ------------------------------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(LM_NAMESPACE)
LM_NAMESPACE_BEGIN(detail)

/*!
    \brief Wraps registration of component instance.
*/
template <typename InterfaceT>
static void reg_comp_wrap(pybind11::object impl_class, const char* name) {
    lm::comp::detail::reg(name, "",
        [impl_class = impl_class]() -> lm::Component* {
            pybind11::gil_scoped_acquire gil;

            // Create instance of python class
            auto instPy = impl_class();
            // We need to keep track of the python object
            // to manage lifetime of the instance
            auto instCpp = instPy.cast<InterfaceT*>();
            // Assignment to std::any increments ref counter.
            comp::detail::Access::ownerRef(instCpp) = instPy;
            // Destruction of instPy decreases ref counter.
            return instCpp;
        },
        [](lm::Component* p) -> void {
            pybind11::gil_scoped_acquire gil;

            // Automatic deref of instPy causes invocation of GC.
            // Note we need one extra deref because one reference is still hold by ownerRef_.
            // The pointer of component is associated with instPy
            // and it will be destructed when it is deallocated by GC.
            auto instPy = std::any_cast<pybind11::object>(comp::detail::Access::ownerRef(p));
            instPy.dec_ref();
            // Prevent further invocation of release function
            comp::detail::Access::release_func(p) = nullptr;
        });
}

/*!
    \brief Cast lm::Component to Python object.
*/
template <typename InterfaceT>
static pybind11::object cast_to_python_object(Component* inst) {
    // We cannot bind lm::comp::create directly because pybind does not support
    // instantiation of unique_ptr with custom deleter. Internally, pybind requires
    // construction of holder_type (e.g., std::unique_ptr) from T*.
    // Instead we associate lifetime of the object with python object.

    // Extract owner if available
    auto& ownerRef = comp::detail::Access::ownerRef(inst);
    if (ownerRef.has_value()) {
        // Returning instantiated pointer incurs another wrapper for the pointer inside pybind.
        // To avoid this, if the instance was created inside python, extract the underlying
        // python object and directly return it.
        auto instPy = std::any_cast<pybind11::object>(ownerRef);
        return instPy;
    }
    // Otherwise inst is C++ instance so return with standard pointer
    // It should work without registered deleter, underlying unique_ptr will take care of it.
    auto* instT = dynamic_cast<InterfaceT*>(inst);
    return pybind11::cast(instT, pybind11::return_value_policy::take_ownership);
}

/*!
    \brief Wraps creation of component instance.
*/
template <typename InterfaceT>
static pybind11::object create_without_construct_wrap(const char* name, const char* loc) {
    auto inst = lm::comp::detail::create_comp(name);
    if (!inst) {
        return pybind11::object();
    }
    lm::comp::detail::Access::loc(inst) = loc;
    return cast_to_python_object<InterfaceT>(inst);
}

/*!
    \brief Creation of component instance with construction.
*/
template <typename InterfaceT>
static pybind11::object create_comp_wrap(const char* name, const char* loc, const Json& prop) {
    auto inst = lm::comp::detail::create_comp(name);
    if (!inst) {
        return pybind11::object();
    }
    lm::comp::detail::Access::loc(inst) = loc;
    inst->construct(prop);
    return cast_to_python_object<InterfaceT>(inst);
}

/*!
    \brief Cast from component type.
*/
template <typename InterfaceT>
static std::optional<InterfaceT*> cast_from(Component* p) {
    return dynamic_cast<InterfaceT*>(p);
}

/*!
    \brief Adds component-related functions to the binding of a component interface.
*/
#define PYLM_DEF_COMP_BIND(InterfaceT) \
     def_static("reg", &LM_NAMESPACE::detail::reg_comp_wrap<InterfaceT>) \
    .def_static("unreg", &LM_NAMESPACE::comp::detail::unreg) \
    .def_static("create_without_construct", \
        &LM_NAMESPACE::detail::create_without_construct_wrap<InterfaceT>) \
    .def_static("create", \
        &LM_NAMESPACE::detail::create_comp_wrap<InterfaceT>) \
    .def_static("cast", \
        &LM_NAMESPACE::detail::cast_from<InterfaceT>, \
        pybind11::return_value_policy::reference);

/*!
    \brief Implement save() and load() function for trampoline helper class.
    This workaround casting facility of pybind11 for Input/OutputArchive.
*/
#define PYLM_SERIALIZE_IMPL(ComponentT) \
    virtual void save(lm::OutputArchive& ar) override { \
        pybind11::gil_scoped_acquire gil; \
        auto overload = pybind11::get_overload(static_cast<const ComponentT*>(this), "save"); \
        if (overload) { \
            auto s = pybind11::detail::cast_safe<std::string>(overload()); \
            ar(s); \
        } \
        else { \
            ComponentT::save(ar); \
        } \
    } \
    virtual void load(lm::InputArchive& ar) override { \
        pybind11::gil_scoped_acquire gil; \
        auto overload = pybind11::get_overload(static_cast<const ComponentT*>(this), "load"); \
        if (overload) { \
            std::string arg; \
            ar(arg); \
            overload(pybind11::bytes(arg)); \
        } \
        else { \
            ComponentT::load(ar); \
        } \
    }

LM_NAMESPACE_END(detail)
LM_NAMESPACE_END(LM_NAMESPACE)

// ------------------------------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \brief Module binder for python interfaces.
*/
class PyBinder : public lm::Component {
public:
    /*!
        \brief Bind to the module.
        \param m Pybind11's python module object.
    */
    virtual void bind(pybind11::module& m) const = 0;
};

LM_NAMESPACE_END(LM_NAMESPACE)