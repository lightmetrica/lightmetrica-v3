/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "common.h"
#include "math.h"
#include <nlohmann/json.hpp>
#include <array>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// JSON type
using json = nlohmann::json;

// Conversion to JSON type
template <typename T>
json jsonCast(T&& v) {
    return detail::JsonCastImpl<T>::cast(std::forward(v));
}

LM_NAMESPACE_BEGIN(detail)

template <typename T>
struct JsonCastImpl;

template <int N, typename T, glm::qualifier Q>
struct JsonCastImpl<glm::vec<N, T, Q>> {
    using ValueT = glm::vec3<N, T, Q>;
    static json cast(ValueT&& v) {
        std::array<T, N> a(&v[0], &v[0] + N);
        return json(a);
    }
};

LM_NAMESPACE_END(detail)
LM_NAMESPACE_END(LM_NAMESPACE)
