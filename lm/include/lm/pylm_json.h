/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "common.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

// Type caster for json type
namespace pybind11::detail {
template <>
struct type_caster<lm::Json> {
public:
    // Register this class as type caster
    PYBIND11_TYPE_CASTER(lm::Json, _("json"));

    // Python -> C++
    bool load(handle src, bool convert) {
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
    template <typename T>
    static handle cast(T&& src, return_value_policy policy, handle parent) {
        using namespace nlohmann::detail;
        // Check types
        switch (src.type()) {
            case value_t::null: {
                return none().release();
            }
            case value_t::boolean: {
                return castToPythonObject<bool>(std::forward<T>(src), policy, std::move(parent));
            }
            case value_t::number_float: {
                return castToPythonObject<double>(std::forward<T>(src), policy, std::move(parent));
            }
            case value_t::number_integer: { [[fallthrough]]; }
            case value_t::number_unsigned: {
                return castToPythonObject<int>(std::forward<T>(src), policy, std::move(parent));
            }
            case value_t::string: {
                return castToPythonObject<std::string>(std::forward<T>(src), policy, std::move(parent));
            }
            case value_t::object: {
                auto policy_key = return_value_policy_override<std::string>::policy(policy);
                auto policy_value = return_value_policy_override<lm::Json>::policy(policy);
                dict d;
                for (lm::Json::iterator it = src.begin(); it != src.end(); ++it) {
                    auto k = reinterpret_steal<object>(
                        make_caster<std::string>::cast(forward_like<T>(it.key()), policy_key, parent));
                    auto v = reinterpret_steal<object>(
                        make_caster<lm::Json>::cast(forward_like<T>(it.value()), policy_value, parent));
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
                        make_caster<lm::Json>::cast(forward_like<T>(element), policy_value, parent));
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
    template <typename U, typename T>
    static handle castToPythonObject(T&& src, return_value_policy policy, handle&& parent) {
        auto p = return_value_policy_override<U>::policy(policy);
        auto v = reinterpret_steal<object>(make_caster<U>::cast(forward_like<T>(src.get<U>()), p, parent));
        if (!v) {
            return handle();
        }
        return v.release();
    }

};
}