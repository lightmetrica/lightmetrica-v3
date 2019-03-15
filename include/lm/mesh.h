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

    \rst
    This component interface represents a triangle mesh,
    respondible for handling or manipulating triangle mesh.
    \endrst
*/
class Mesh : public Component {
public:
    /*!
        \brief Vertex of a triangle.

        \rst
        Represents geometry information associated with a vertice of a triangle.
        \endrst
    */
    struct Point {
        Vec3 p;     //!< Position.
        Vec3 n;     //!< Normal.
        Vec2 t;     //!< Texture coordinates.
    };

    /*!
        \brief Triangle.

        \rst
        Represents a triangle composed of three vertices.
        \endrst
    */
    struct Tri {
        Point p1;   //!< First vertex.
        Point p2;   //!< Second vertex
        Point p3;   //!< Third vertex.
    };

public:
    /*!
        \brief Callback function for processing a triangle.
        \param face Face index.
        \param tri Triangle.

        \rst
        The function of this type is used as a callback function to process a single triangle,
        used as an argument of :cpp:func:`lm::Mesh::foreachTriangle` function.
        \endrst
    */
    using ProcessTriangleFunc = std::function<void(int face, const Tri& tri)>;

    /*!
        \brief Iterate triangles in the mesh.
        \param processTriangle Callback function to process a triangle.

        \rst
        This function enumerates all triangles inside the mesh.
        A specified callback function is called for each triangle.
        \endrst
    */
    virtual void foreachTriangle(const ProcessTriangleFunc& processTriangle) const {
        LM_UNUSED(processTriangle);
        LM_UNREACHABLE();
    }

    /*!
        \brief Get triangle by face index.
    */
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
