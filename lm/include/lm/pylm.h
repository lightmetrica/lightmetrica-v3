/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "lm.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <pybind11/numpy.h>

// ----------------------------------------------------------------------------

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
                return castToPythonObject<bool>(src, policy, std::move(parent));
            }
            case value_t::number_float: {
                return castToPythonObject<double>(src, policy, std::move(parent));
            }
            case value_t::number_integer: { [[fallthrough]]; }
            case value_t::number_unsigned: {
                return castToPythonObject<int>(src, policy, std::move(parent));
            }
            case value_t::string: {
                return castToPythonObject<std::string>(src, policy, std::move(parent));
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
                LM_TBA_RUNTIME();
                break;
            }
        }
        LM_UNREACHABLE();
        return handle();
    }

private:
    template <typename U>
    static handle castToPythonObject(const lm::Json& src, return_value_policy policy, handle&& parent) {
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
    static handle cast(VecT&& src, return_value_policy policy, handle parent) {
        LM_UNUSED(policy);
        // Create numpy array from VecT
        array a(
            {N},          // Shapes
            {sizeof(T)},  // Strides
            &src.x,       // Data
            parent        // Parent handle
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
        glm::transpose(value);

        return true;
    }

    // C++ -> Python
    static handle cast(MatT&& src, return_value_policy policy, handle parent) {
        LM_UNUSED(policy);
        // Create numpy array from MatT
        src = glm::transpose(src);
        array a(
            {R, C},                    // Shapes
            {R*sizeof(T), sizeof(T)},  // Strides
            &src[0].x,                 // Data
            parent                     // Parent handle
        );
        return a.release();
    }

};

LM_NAMESPACE_END(pybind11::detail)

// ----------------------------------------------------------------------------

// Add component's unique_ptr as internal holder type
PYBIND11_DECLARE_HOLDER_TYPE(T, lm::Component::Ptr<T>, true);

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(LM_NAMESPACE)
LM_NAMESPACE_BEGIN(detail)

// Allows to access private members of Component
class ComponentAccess {
public:
    static std::any& ownerRef(Component& comp) {
        return comp.ownerRef_;
    }
    static Component::ReleaseFunction& releaseFunc(Component& comp) {
        return comp.releaseFunc_;
    }
};

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
            detail::ComponentAccess::ownerRef(*instCpp) = instPy;
            // Destruction of instPy decreases ref counter.
            return instCpp;
        },
        [](lm::Component* p) -> void {
            // Automatic deref of instPy causes invocation of GC.
            // Note we need one extra deref because one reference is still hold by ownerRef_.
            // The pointer of component is associated with instPy
            // and it will be destructed when it is deallocated by GC.
            auto instPy = std::any_cast<pybind11::object>(detail::ComponentAccess::ownerRef(*p));
            instPy.dec_ref();
            // Prevent further invocation of release function
            detail::ComponentAccess::releaseFunc(*p) = nullptr;
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
    auto& ownerRef = detail::ComponentAccess::ownerRef(*inst);
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
    \brief Adds component-related functions to the binding of a component interface.
*/
#define PYLM_DEF_COMP_BIND(InterfaceT) \
     def_static("reg", &LM_NAMESPACE::detail::regCompWrap<InterfaceT>) \
    .def_static("unreg", &LM_NAMESPACE::comp::detail::unreg) \
    .def_static("create", \
        pybind11::overload_cast<const char*, LM_NAMESPACE::Component*>( \
            &LM_NAMESPACE::detail::createCompWrap<InterfaceT>)) \
    .def_static("create", \
        pybind11::overload_cast<const char*, LM_NAMESPACE::Component*, const LM_NAMESPACE::Json&>( \
            &LM_NAMESPACE::detail::createCompWrap<InterfaceT>))

// Trampoline class for lm::Component
class Component_Py final : public Component {
public:
    virtual bool construct(const Json& prop) override {
        PYBIND11_OVERLOAD(bool, Component, construct, prop);
    }
};

LM_NAMESPACE_END(detail)

// ----------------------------------------------------------------------------

//LM_NAMESPACE_BEGIN(log)
//LM_NAMESPACE_BEGIN(detail)
//
//// logger.h
//class LoggerContext_Py : public LoggerContext {
//    virtual bool construct(const Json& prop) override {
//        PYBIND11_OVERLOAD_PURE(bool, LoggerContext, construct, prop);
//    }
//    virtual void log(lm::log::LogLevel level, const char* filename, int line, const char* message) override {
//        PYBIND11_OVERLOAD_PURE(void, LoggerContext, log, level, filename, line, message);
//    }
//    virtual void updateIndentation(int n) override {
//        PYBIND11_OVERLOAD_PURE(void, LoggerContext, updateIndentation, n);
//    }
//};
//
//LM_NAMESPACE_END(detail)
//LM_NAMESPACE_END(log)

// ----------------------------------------------------------------------------

/*!
    \brief Binds Lightmetrica to the specified module.
*/
static void bind(pybind11::module& m) {
    using namespace pybind11::literals;

    // ------------------------------------------------------------------------

    // component.h
    pybind11::class_<Component, detail::Component_Py, Component::Ptr<Component>>(m, "Component")
        .def(pybind11::init<>())
        .def("construct", &Component::construct)
        .def("parent", &Component::parent)
        .PYLM_DEF_COMP_BIND(Component);

    {
        // Namespaces are handled as submodules
        auto sm = m.def_submodule("comp");
        auto sm_detail = sm.def_submodule("detail");

        // Component API
        sm_detail.def("loadPlugin", &comp::detail::loadPlugin);
        sm_detail.def("loadPlugins", &comp::detail::loadPlugins);
        sm_detail.def("unloadPlugins", &comp::detail::unloadPlugins);
        sm_detail.def("foreachRegistered", &comp::detail::foreachRegistered);
    }

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

LM_NAMESPACE_END(LM_NAMESPACE)
