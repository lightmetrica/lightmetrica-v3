/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "component.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \brief Renderer component interface.
*/
class Renderer : public Component {
public:
    /*!
        \brief Process rendering.
    */
    virtual void render(const Scene* scene) const = 0;
};

LM_NAMESPACE_END(LM_NAMESPACE)
