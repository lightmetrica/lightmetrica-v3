/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "component.h"
#include "math.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \addtogroup light
    @{
*/

/*!
    \brief Light.
*/
class Light : public Component {
public:
    /*!
        \brief Sample a position on the light.
    */
    virtual std::optional<LightSample> sampleLight(Rng& rng, const SurfacePoint& sp, const Transform& transform) const = 0;

    /*!
        \brief Evaluate pdf for light sampling in projected solid angle measure.
    */
    virtual Float pdfLight(const SurfacePoint& sp, const SurfacePoint& spL, const Transform& transform, Vec3 wo) const = 0;

    /*!
        \brief Check if the light is specular.
    */
    virtual bool isSpecular(const SurfacePoint& sp) const = 0;

    /*!
        \brief Check if the light is on infinite position (e.g., environment light).
    */
    virtual bool isInfinite() const = 0;

    /*!
        \brief Evaluate luminance.
    */
    virtual Vec3 eval(const SurfacePoint& sp, Vec3 wo) const = 0;
};

/*!
    @}
*/

LM_NAMESPACE_END(LM_NAMESPACE)
