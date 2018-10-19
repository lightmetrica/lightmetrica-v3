/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "detail/common.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)
LM_NAMESPACE_BEGIN(api)

/*!
    \brief Initializes the renderer.
*/
LM_PUBLIC_API void init();

/*!
    \brief Gracefully finalizes the renderer.
*/
LM_PUBLIC_API void shutdown();

// ----------------------------------------------------------------------------

/*!
    \brief Render image based on current configuration.

    Example:
*/
LM_PUBLIC_API int render();

// ----------------------------------------------------------------------------

/*!
    \brief Creates a new primitive and add it to the scene.
           Returns the ID of the primitive.

    Example:

    This example queries the ID of the primitive named as `test`.

    \snippet test/test_api.cpp primitive example

    \param name Name of the primitive.
    \return     ID of the primitive.
*/
LM_PUBLIC_API int primitive(const std::string& name);

LM_NAMESPACE_END(api)
LM_NAMESPACE_END(LM_NAMESPACE)

