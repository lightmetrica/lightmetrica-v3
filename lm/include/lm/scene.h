/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "detail/component.h"
#include "detail/forward.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \brief Scene component interface.
*/
class Scene : public Component {
private:
    Scene() = default;
    virtual ~Scene() = default;
    LM_DISABLE_COPY_AND_MOVE(Scene);

public:

};

LM_NAMESPACE_END(LM_NAMESPACE)
