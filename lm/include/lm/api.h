/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "common.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    Initializes the renderer.
*/
LM_PUBLIC_API void init();

/*!
    Gracefully finalizes the renderer.
*/
LM_PUBLIC_API void shutdown();

// ----------------------------------------------------------------------------

/*!
    Render image based on current configuration.

    Example
    =======

*/
LM_PUBLIC_API int render();

// ----------------------------------------------------------------------------

/*!
    Creates a new primitive and add it to the scene.
    Returns the ID of the primitive.

    Example
    =======

    This example queries the ID of the primitive named as `test`.

    \snippet test/main.cpp primitive example

    \param name Name of the primitive.
    \return     ID of the primitive.
*/
LM_PUBLIC_API int primitive(const std::string& name);

LM_NAMESPACE_END(LM_NAMESPACE)

