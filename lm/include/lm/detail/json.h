/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "common.h"
#include "math.h"
#include "../logger.h"
#include <nlohmann/json.hpp>
#include <array>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// JSON type
using json = nlohmann::json;

// Conversion to JSON type
template <typename T>
json castToJson(T&& v) {
    return detail::JsonCastImpl<T>::castToJson(std::forward(v));
}

template <typename T>
T castFromJson(const json& j) {
    return detail::JsonCastImpl<T>::castFromJson(j);
}

LM_NAMESPACE_BEGIN(detail)

template <typename T>
struct JsonCastImpl;

template <int N, typename T, glm::qualifier Q>
struct JsonCastImpl<glm::vec<N, T, Q>> {
    using ValueT = glm::vec3<N, T, Q>;
    static json castToJson(ValueT&& v) {
        std::array<T, N> a;
        for (int i = 0; i < N; i++) {
            a[i] = static_cast<json::number_float_t>(v[i]);
        }
        return json(a);
    }
    static ValueT castFromJson(const json& j) {
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
