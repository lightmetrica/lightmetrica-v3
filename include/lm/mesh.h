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
        \brief Vertex of a mesh.
    */
    struct Point {
        Vec3 p;     // Position
        Vec3 n;     // Normal
        Vec2 t;     // Texture coordinates
    };

public:
    /*!
        \brief Iterate triangles in the mesh.
    */
    using ProcessTriangleFunc = std::function<void(int face, Point p1, Point p2, Point p3)>;
    virtual void foreachTriangle(const ProcessTriangleFunc& processTriangle) const {
        LM_UNUSED(processTriangle);
        LM_UNREACHABLE();
    }

    /*!
        \brief Get triangle by face index.
    */
    struct Tri { Vec3 p1, p2, p3; };
    virtual Tri triangleAt(int face) const {
        LM_UNUSED(face);
        LM_UNREACHABLE_RETURN();
    }

    /*!
        \brief Compute surface geometry information at a point.
    */
    virtual Point surfacePoint(int face, Vec2 uv) const {
        LM_UNUSED(face, uv);
        LM_UNREACHABLE_RETURN();
    }
};

/*!
    @}
*/

LM_NAMESPACE_END(LM_NAMESPACE)
