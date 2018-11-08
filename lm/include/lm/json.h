/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "common.h"
#include "math.h"
#include <array>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// Conversion to JSON type
template <typename T>
Json castToJson(const T& v) {
    return detail::JsonCastImpl<T>::castToJson(v);
}

template <typename T>
T castFromJson(const Json& j) {
    return detail::JsonCastImpl<T>::castFromJson(j);
}

/*!
    \brief Parse positional command line arguments.
*/
template <size_t N>
Json parsePositionalArgs(int argc, char** argv, const std::string& temp) {
    return Json::parse(detail::formatWithStringVector<N>(temp, { argv + 1, argv + argc }));
}

LM_NAMESPACE_BEGIN(detail)

template <typename T>
struct JsonCastImpl;

template <int N, typename T, glm::qualifier Q>
struct JsonCastImpl<glm::vec<N, T, Q>> {
    using ValueT = glm::vec<N, T, Q>;
    static Json castToJson(const ValueT& v) {
        std::array<T, N> a;
        for (int i = 0; i < N; i++) {
            #pragma warning (push)
            #pragma warning (disable:4244)
            a[i] = static_cast<Json::number_float_t>(v[i]);
            #pragma warning (pop)
        }
        return Json(a);
    }
    static ValueT castFromJson(const Json& j) {
        if (!j.is_array()) {
            return {};
        }
        ValueT v;
        for (int i = 0; i < N; i++) {
            v[i] = static_cast<T>(j[i]);
        }
        return v;
    }
};

// https://stackoverflow.com/questions/48875467/how-to-pass-not-variadic-values-to-fmtformat
template <std::size_t ... Is>
std::string formatWithStringVector(const std::string& format, const std::vector<std::string>& v, std::index_sequence<Is...>) {
    return fmt::format(format, v[Is]...);
}

template <std::size_t N>
std::string formatWithStringVector(const std::string& format, const std::vector<std::string>& v) {
    return formatWithStringVector(format, v, std::make_index_sequence<N>());
}

LM_NAMESPACE_END(detail)
LM_NAMESPACE_END(LM_NAMESPACE)
