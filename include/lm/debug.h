/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "component.h"
#include "math.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)
LM_NAMESPACE_BEGIN(debug)

/*!
    \addtogroup debug
    @{
*/

/*!
    \brief Poll floating-point value.
*/
LM_PUBLIC_API void pollFloat(const std::string& name, Float val);

/*!
    \brief Function being called on polling floating-point values.
*/
using OnPollFloatFunc = std::function<void(const std::string& name, Float val)>;

/*!
    \brief Register polling function for floating-point values.
*/
LM_PUBLIC_API void regOnPollFloat(const OnPollFloatFunc& onPollFloat);

/*!
    @}
*/

LM_NAMESPACE_END(debug)
LM_NAMESPACE_END(LM_NAMESPACE)
