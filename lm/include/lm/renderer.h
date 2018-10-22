/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "detail/component.h"
#include "detail/forward.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \brief Renderer component interface.
*/
class Renderer : public Component {
public:
    Renderer() = default;
    virtual ~Renderer() = default;
    LM_DISABLE_COPY_AND_MOVE(Renderer);

public:
    /*!
        \brief Process rendering.
    */
    virtual void render(const Scene& scene) const = 0;
};

LM_NAMESPACE_END(LM_NAMESPACE)
