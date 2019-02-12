/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "component.h"
#include "math.h"
#include "surface.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \addtogroup light
    @{
*/

/*!
    \brief Result of Light::sample() function.
*/
struct LightRaySample {
    PointGeometry geom;   // Sampled geometry information
    Vec3 wo;              // Sampled direction
    Vec3 weight;          // Contribution divided by probability
};

/*!
    \brief Light.
*/
class Light : public Component {
public:
    /*!
        \brief Sample a position on the light.
    */
    virtual std::optional<LightRaySample> sample(Rng& rng, const PointGeometry& geom, const Transform& transform) const = 0;

    /*!
        \brief Evaluate pdf for light sampling in projected solid angle measure.
    */
    virtual Float pdf(const PointGeometry& geom, const PointGeometry& geomL, const Transform& transform, Vec3 wo) const = 0;

    /*!
        \brief Check if the light is specular.
    */
    virtual bool isSpecular(const PointGeometry& geom) const = 0;

    /*!
        \brief Check if the light is on infinite position (e.g., environment light).
    */
    virtual bool isInfinite() const = 0;

    /*!
        \brief Evaluate luminance.
    */
    virtual Vec3 eval(const PointGeometry& geom, Vec3 wo) const = 0;
};

/*!
    @}
*/

LM_NAMESPACE_END(LM_NAMESPACE)
