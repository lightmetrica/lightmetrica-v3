/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "common.h"
#include "../math.h"
#include "../logger.h"
#include <nlohmann/json.hpp>
#include <array>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// JSON type
using Json = nlohmann::basic_json<
    std::unordered_map, // Object type
    std::vector,        // Arrray type
    std::string,        // String type
    bool,               // Boolean type
    std::int64_t,       // Signed integer type
    std::uint64_t,      // Unsigned integer type
    Float,              // Floating point type
    std::allocator,
    nlohmann::adl_serializer>;

// Conversion to JSON type
template <typename T>
Json castToJson(T&& v) {
    return detail::JsonCastImpl<T>::castToJson(std::forward<T>(v));
}

template <typename T>
T castFromJson(const Json& j) {
    return detail::JsonCastImpl<T>::castFromJson(j);
}

LM_NAMESPACE_BEGIN(detail)

template <typename T>
struct JsonCastImpl;

template <int N, typename T, glm::qualifier Q>
struct JsonCastImpl<glm::vec<N, T, Q>> {
    using ValueT = glm::vec<N, T, Q>;
    static Json castToJson(ValueT&& v) {
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
            LM_ERROR("Invalid cast: Array type is required.");
            return {};
        }
        ValueT v;
        for (int i = 0; i < N; i++) {
            v[i] = static_cast<T>(j[i]);
        }
        return v;
    }
};

LM_NAMESPACE_END(detail)
LM_NAMESPACE_END(LM_NAMESPACE)
