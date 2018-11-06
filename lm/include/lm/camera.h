/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "detail/component.h"
#include "math.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \brief Camera.
*/
class Camera : public Component {
public:
    /*!
        \brief Generate a primary ray with the corresponding raster position.
    */
    virtual Ray primaryRay(Vec2 rp) const = 0;

    /*!
        \brief Sample a primary ray within the given raster window.
    */
    virtual std::optional<RaySample> samplePrimaryRay(Rng& rng, Vec4 window) const = 0;
};

LM_NAMESPACE_END(LM_NAMESPACE)
