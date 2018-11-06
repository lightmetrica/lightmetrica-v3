/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "common.h"
#include <nlohmann/json_fwd.hpp>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// ----------------------------------------------------------------------------

// assets.h
class Assets;

// mesh.h
class Mesh;

// material.h
class Material;

// light.h
class Light;

// camera.h
class Camera;

// scene.h
struct SurfacePoint;
struct RaySample;
class Scene;

// renderer.h
class Renderer;

// Some detailed classes
LM_FORWARD_DECLARE_WITH_NAMESPACE(comp::detail, class Impl)
LM_FORWARD_DECLARE_WITH_NAMESPACE(py::detail, class Impl)

// ----------------------------------------------------------------------------

// Default floating point type
using Float = float;

// JSON type
using Json = nlohmann::basic_json<
    std::map,           // Object type
    std::vector,        // Arrray type
    std::string,        // String type
    bool,               // Boolean type
    std::int64_t,       // Signed integer type
    std::uint64_t,      // Unsigned integer type
    Float,              // Floating point type
    std::allocator,
    nlohmann::adl_serializer>;

// ----------------------------------------------------------------------------

LM_NAMESPACE_END(LM_NAMESPACE)
