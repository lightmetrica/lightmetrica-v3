/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "detail/component.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \brief Collection of assets.
*/
class Assets : public Component {
public:
    Assets() = default;
    virtual ~Assets() = default;
    LM_DISABLE_COPY_AND_MOVE(Assets);

public:


};

LM_NAMESPACE_END(LM_NAMESPACE)
