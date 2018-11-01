/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "common.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

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
class Scene;

// renderer.h
class Renderer;

// Some detailed classes
LM_FORWARD_DECLARE_WITH_NAMESPACE(comp::detail, class Impl)
LM_FORWARD_DECLARE_WITH_NAMESPACE(py::detail, class Impl)

LM_NAMESPACE_END(LM_NAMESPACE)