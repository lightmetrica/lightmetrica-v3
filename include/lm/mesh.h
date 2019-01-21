/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "component.h"
#include "math.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \addtogroup mesh
    @{
*/

/*!
    \brief Triangle mesh.
*/
class Mesh : public Component {
public:
    /*!
        \brief Iterate triangles in the mesh.
    */
    using ProcessTriangleFunc = std::function<void(int face, Vec3 p1, Vec3 p2, Vec3 p3)>;
    virtual void foreachTriangle(const ProcessTriangleFunc& processTriangle) const = 0;

    /*!
        \brief Get triangle by face index.
    */
    struct Tri { Vec3 p1, p2, p3; };
    virtual Tri triangleAt(int face) const = 0;

    /*!
        \brief Compute surface geometry information at a point.
    */
    struct Point {
        Vec3 p;     // Position
        Vec3 n;     // Normal
        Vec2 t;     // Texture coordinates
    };
    virtual Point surfacePoint(int face, Vec2 uv) const = 0;
};

/*!
    @}
*/

LM_NAMESPACE_END(LM_NAMESPACE)
