/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "component.h"
#include "math.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \addtogroup accel
    @{
*/

/*!
    \brief Ray-triangles acceleration structure.

    \rst
    Interfaces acceleration structure for ray-triangles intersection.
    We provided several tests to check validity or performance of the implementations.
    See ``functest`` directory for detail.
    \endrst
*/
class Accel : public Component {
public:
    /*!
        \brief Build acceleration structure.
        \param scene Input scene.

        \rst
        Builds the acceleration structure from the primitives inside the given scene.
        When a primitive inside the scene is updated by addition or modification,
        you need to call the function again to update the structure.
        \endrst
    */
    virtual void build(const Scene& scene) = 0;

    /*!
        \brief Hit result.

        \rst
        Shows information associated to the hit point of the ray.
        More additional information like surface positions can be obtained
        from querying appropriate data types from these information.
        \endrst
    */
    struct Hit {
        Float t;                    //!< Distance to the hit point.
        Vec2 uv;                    //!< Barycentric coordinates.
        Transform globalTransform;  //!< Global transformation.
        int group;                  //!< Group index.
        int primitive;              //!< Primitive index.
        int face;                   //!< Face index.
    };

    /*!
        \brief Compute closest intersection point.
        \param ray Ray.
        \param tmin Lower valid range of the ray.
        \param tmax Higher valid range of the ray.

        \rst
        Finds the closest intersecion point in the direction specified by ``ray``.
        The validity of the ray segment is speicified by the range ``[tmin, tmax]``
        measured from the origin of the ray.
        If no intersection is found, the function returns nullopt.
        \endrst
    */
    virtual std::optional<Hit> intersect(Ray ray, Float tmin, Float tmax) const = 0;
};

/*!
    @}
*/

LM_NAMESPACE_END(LM_NAMESPACE)
