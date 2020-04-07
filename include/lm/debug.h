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
    \brief Poll JSON value.
    \param val Value.
*/
LM_PUBLIC_API void poll(const Json& val);

/*!
    \brief Function being called on polling JSON value.
    \param val Value.
*/
using OnPollFunc = std::function<void(const Json& val)>;

/*!
    \brief Register polling function for floating-point values.
    \param func Callback function.
*/
LM_PUBLIC_API void reg_on_poll(const OnPollFunc& func);

/*!
    \brief Attach to debugger.

    \rst
    Attach the process to Visual Studio Debugger.
    This function is only available in Windows environment.
    \endrst
*/
LM_PUBLIC_API void attach_to_debugger();

/*!
    \brief Prints assets managed by the framework.
    \param visualize_weak_refs Visualize weak references if enabled.

    \rst
    This function visualizes a tree of assets managed by the framework.
    If ``visualize_weak_refs`` flag enabled, weak references are also visualized.
    \endrst
*/
LM_PUBLIC_API void print_asset_tree(bool visualize_weak_refs);

/*!
    @}
*/

LM_NAMESPACE_END(debug)
LM_NAMESPACE_END(LM_NAMESPACE)
