/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "component.h"
#include "math.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \brief Light.
*/
class Light : public Component {
public:
    /*!
        \brief Check if the light is specular.
    */
    virtual bool isSpecular(const SurfacePoint& sp) const = 0;

    /*!
        \brief Evaluate Luminance.
    */
    virtual Vec3 eval(const SurfacePoint& sp, Vec3 wo) const = 0;
};

LM_NAMESPACE_END(LM_NAMESPACE)
