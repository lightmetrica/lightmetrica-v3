/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "detail/component.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \brief Material.
*/
class Material : public Component {
public:
    /*!
        \brief Check if the material is specular.
    */
    virtual bool isSpecular() const = 0;

    /*!
        \brief Sample a ray given surface point and incident direction.
    */
    virtual std::optional<RaySample> sampleRay(Rng& rng, const SurfacePoint& sp, Vec3 wi) const = 0;
};

LM_NAMESPACE_END(LM_NAMESPACE)
