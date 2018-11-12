/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "common.h"
#include "math.h"
#include <array>

// ----------------------------------------------------------------------------

/*!
    \brief String literal for JSON types.
*/
inline lm::Json operator "" _lmJson(const char* s, size_t n) {
    return lm::Json::parse(s, s + n);
}

// ----------------------------------------------------------------------------

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
            throw std::runtime_error(
                fmt::format("Invalid JSON type [expected='array', actual='{}']", j.type_name()));
        }
        if (j.size() != N) {
            throw std::runtime_error(
                fmt::format("Invalid number of elements [expected={}, actual={}]", N, j.size()));
        }
        for (int i = 0; i < N; i++) {
            v[i] = static_cast<T>(j[i]);
        }
    }
};

LM_NAMESPACE_END(nlohmann)

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \brief Parse positional command line arguments.
*/
template <size_t N>
Json parsePositionalArgs(int argc, char** argv, const std::string& temp) {
    return Json::parse(detail::formatWithStringVector<N>(temp, { argv + 1, argv + argc }));
}

/*!
    \brief Get the value inside the element with default.
*/
template <typename T>
T valueOr(const Json& j, const std::string& name, T&& default) {
    if (const auto it = j.find(name); it != j.end()) {
        return *it;
    }
    return default;
}

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(detail)

// https://stackoverflow.com/questions/48875467/how-to-pass-not-variadic-values-to-fmtformat
template <std::size_t ... Is>
std::string formatWithStringVector(const std::string& format, const std::vector<std::string>& v, std::index_sequence<Is...>) {
    return fmt::format(format, v[Is]...);
}

template <std::size_t N>
std::string formatWithStringVector(const std::string& format, const std::vector<std::string>& v) {
    return formatWithStringVector(format, v, std::make_index_sequence<N>());
}

// ----------------------------------------------------------------------------

LM_NAMESPACE_END(detail)
LM_NAMESPACE_END(LM_NAMESPACE)
