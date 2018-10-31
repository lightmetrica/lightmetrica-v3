/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "detail/component.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Mesh : public Component {
public:
    /*!
        \brief Iterate transformed triangles in the mesh.
    */
    virtual void foreachTriangle(Mat4 transform, const std::function<void(Vec3 p1, Vec3 p2, Vec3 p3)>& processTriangle) const = 0;

    /*!
        \brief Compute surface geometry information at a point.
    */
    virtual SurfacePoint surfacePoint(int face, Vec2 uv) const = 0;
};

LM_NAMESPACE_END(LM_NAMESPACE)
