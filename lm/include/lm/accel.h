/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "detail/component.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \brief Ray-triangles acceleration structure.
*/
class Accel : public Component {
public:
    /*!
        \brief Build acceleration structure.
    */
    virtual void build(const Scene& scene) = 0;

    /*!
        \brief Hit information.
    */
    struct Hit {
        Float t;        // Distance to the hit point
        Vec2 uv;        // Barycentric coordinates
        int primitive;  // Primitive index
        int face;       // Face index
    };

    /*!
        \brief Compute closest intersection point.
    */
    virtual std::optional<Hit> intersect(Ray ray, Float tmin, Float tmax) const = 0;
};

LM_NAMESPACE_END(LM_NAMESPACE)
