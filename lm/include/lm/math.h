/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include  "detail/common.h"
#include <glm/glm.hpp>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// Default floating point type
using F = float;

// Default math types deligated to glm library
using vec2 = glm::tvec2<F>;
using vec3 = glm::tvec3<F>;
using vec4 = glm::tvec4<F>;
using mat3 = glm::tmat3x3<F>;
using mat4 = glm::tmat4x4<F>;

LM_NAMESPACE_END(LM_NAMESPACE)
