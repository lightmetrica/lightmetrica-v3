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
    virtual void build(const Scene& scene) const = 0;
};

LM_NAMESPACE_END(LM_NAMESPACE)
