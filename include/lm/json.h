/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "component.h"
#include "jsontype.h"
#include "math.h"
#include "exception.h"
#include <sstream>
#include <array>

// ------------------------------------------------------------------------------------------------

/*!
    \addtogroup json
    @{
*/

/*!
    \brief String literal for JSON types.
*/
inline lm::Json operator "" _lmJson(const char* s, size_t n) {
    return lm::Json::parse(s, s + n);
}

/*!
    @}
*/

// ------------------------------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(nlohmann)

template <int N, typename T, glm::qualifier Q>
struct adl_serializer<glm::vec<N, T, Q>> {
    using VecT = glm::vec<N, T, Q>;
    static void to_json(lm::Json& j, const VecT& v) {
        std::array<T, N> a;
        for (int i = 0; i < N; i++) {
            a[i] = static_cast<lm::Json::number_float_t>(v[i]);
        }
        j = std::move(a);
    }
    static void from_json(const lm::Json& j, VecT& v) {
        if (!j.is_array()) {
            LM_THROW_EXCEPTION(lm::Error::InvalidArgument,
                "Invalid JSON type [expected='array', actual='{}']", j.type_name());
        }
        if (j.size() != N) {
            LM_THROW_EXCEPTION(lm::Error::InvalidArgument,
                "Invalid number of elements [expected={}, actual={}]", N, j.size());
        }
        for (int i = 0; i < N; i++) {
            v[i] = static_cast<T>(j[i]);
        }
    }
};

template <int C, int R, typename T, glm::qualifier Q>
struct adl_serializer<glm::mat<C, R, T, Q>> {
    using MatT = glm::mat<C, R, T, Q>;
    static void to_json(lm::Json& json, const MatT& v) {
        std::array<T, C*R> a;
        for (int i = 0; i < C; i++) {
            for (int j = 0; j < R; j++) {
                a[i*R+j] = static_cast<lm::Json::number_float_t>(v[i][j]);
            }
        }
        json = std::move(a);
    }
    static void from_json(const lm::Json& json, MatT& v) {
        if (!json.is_array()) {
            LM_THROW_EXCEPTION(lm::Error::InvalidArgument,
                "Invalid JSON type [expected='array', actual='{}']", json.type_name());
        }
        if (json.size() != C*R) {
            LM_THROW_EXCEPTION(lm::Error::InvalidArgument,
                "Invalid number of elements [expected={}, actual={}]", C*R, json.size());
        }
        for (int i = 0; i < C; i++) {
            for (int j = 0; j < R; j++) {
                v[i][j] = static_cast<T>(json[i*R+j]);
            }
        }
    }
};

template <typename T>
struct adl_serializer<T, std::enable_if_t<std::is_pointer_v<T>, void>> {
    template <typename BasicJsonType>
    static void to_json(BasicJsonType& j, T v) {
        std::stringstream ss;
        ss << (void*)v;
        std::string s = ss.str();
        j = s;
    }

    template <typename BasicJsonType>
    static void from_json(const BasicJsonType& j, T& v) {
        std::string s = j;
        std::stringstream ss(s);
        void* t; ss >> t;
        v = (T)t;
    }
};

LM_NAMESPACE_END(nlohmann)

// ------------------------------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(LM_NAMESPACE)
LM_NAMESPACE_BEGIN(json)

// ------------------------------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(detail)

// https://stackoverflow.com/questions/48875467/how-to-pass-not-variadic-values-to-fmtformat
template <std::size_t ... Is>
std::string format_with_string_vector(const std::string& format, const std::vector<std::string>& v, std::index_sequence<Is...>) {
    return fmt::format(format, v[Is]...);
}

template <std::size_t N>
std::string format_with_string_vector(const std::string& format, const std::vector<std::string>& v) {
    return format_with_string_vector(format, v, std::make_index_sequence<N>());
}

LM_NAMESPACE_END(detail)

// ------------------------------------------------------------------------------------------------

/*!
    \addtogroup json
    @{
*/

/*!
    \brief Merge two parameters.
	\param j1 Parameters.
	\param j2 Parameters.
    
    \rst
    If the same key is used, the value in `j1` has priority.
    \endrst
*/
LM_INLINE Json merge(const Json& j1, const Json& j2) {
    assert(j1.is_object());
    assert(j2.is_object());
    Json t(j1);
    for (const auto& e : j2.items()) {
        if (t.find(e.key()) != t.end()) {
            continue;
        }
        t[e.key()] = e.value();
    }
    return t;
}

/*!
    \brief Parse positional command line arguments.
    \param argc Number of arguments.
    \param argv Vector of arguments.
    \param temp Json object with templates.
    \see `example/raycast.cpp`

    \rst
    Convenience function to parse positional command line arguments.
    The format can be specified by Json object with formats.
    A sequence of ``{}`` inside the string ``temp`` are replaced with
    positional command line arguments and parsed as a Json object.
    \endrst
*/
template <size_t N>
Json parse_positional_args(int argc, char** argv, const std::string& temp) {
    return Json::parse(detail::format_with_string_vector<N>(temp, { argv + 1, argv + argc }));
}

/*!
    \brief Get value inside JSON element.
    \param j Json object.
    \param name Name of the element.

    \rst
    This function will cause an exception
    if JSON object does not contain the element.
    \endrst
*/
template <typename T>
T value(const Json& j, const std::string& name) {
    if (const auto it = j.find(name); it != j.end()) {
        return *it;
    }
    LM_THROW_EXCEPTION(Error::InvalidArgument, "Missing property [name='{}']", name);
}

/*!
    \brief Get value inside JSON element with default.
    \param j Json object.
    \param name Name of the element.
    \param def Default value.
*/
template <typename T>
T value(const Json& j, const std::string& name, T&& def) {
    if (const auto it = j.find(name); it != j.end()) {
        return *it;
    }
    return def;
}

/*!
    \brief Get value inside JSON element as component.
    \param j Json object.
    \param name Name of the element.
    
    \rst
    This function returns a component pointer
    referenced by a component locator specified by a JSON element.
    If failed, this function throws an exception.
    \endrst
*/
template <typename T>
T* comp_ref(const Json& j, const std::string& name) {
    const auto it = j.find(name);
    if (it == j.end()) {
        LM_THROW_EXCEPTION(Error::InvalidArgument, "Missing property [name='{}']", name);
    }
    if (!it->is_string()) {
        LM_THROW_EXCEPTION(Error::InvalidArgument, "Property must be string [name='{}']", name);
    }
    const std::string ref = *it;
    auto* p = comp::get<T>(ref);
    if (!p) {
        LM_THROW_EXCEPTION(Error::InvalidArgument, "Invalid componen reference [name='{}', ref='{}']", name, ref);
    }
    return p;
}

/*!
    \brief Get value inside JSON element as component (if failed, nullptr).
    \param j Json object.
    \param name Name of the element.

    \rst
    This function returns a component pointer
    referenced by a component locator specified by a JSON element.
    Unlike :cpp:func:`lm::json::comp_ref`, this function returns ``nullptr`` if failed.
    \endrst
*/
template <typename T>
T* comp_ref_or_nullptr(const Json& j, const std::string& name) {
    const auto it = j.find(name);
    if (it == j.end()) {
        return nullptr;
    }
    if (!it->is_string()) {
        return nullptr;
    }
    const std::string ref = *it;
    auto* p = comp::get<T>(ref);
    if (!p) {
        return nullptr;
    }
    return p;
}

/*!
    \brief Get value inside JSON element with std::optional.
    \param j Json object.
    \param name Name of the element.

    \rst
    This variant of value function returns std::nullopt
    if the json object doesn't contain the element with given name.
    \endrst
*/
template <typename T>
std::optional<T> value_or_none(const Json& j, const std::string& name) {
    if (const auto it = j.find(name); it != j.end()) {
        T v = *it;
        return v;
    }
    return {};
}

/*!
    @}
*/

LM_NAMESPACE_END(json)
LM_NAMESPACE_END(LM_NAMESPACE)
