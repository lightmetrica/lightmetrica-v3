/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "detail/component.h"
#include "math.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \brief Initializes the renderer.
*/
LM_PUBLIC_API void init();

/*!
    \brief Shutdown the renderer.
*/
LM_PUBLIC_API void shutdown();

/*!
    \brief Create an asset.
*/
LM_PUBLIC_API void asset(const std::string& name, const json& params);

/*!
    \brief Creates a new primitive and add it to the scene.
           Returns the ID of the primitive.

    Example:

    This example queries the ID of the primitive named as `test`.

    \snippet test/test_api.cpp primitive example

    \param name Name of the primitive.
    \return     ID of the primitive.
*/
LM_PUBLIC_API void primitive(
    const std::string& name, glm::mat4 transform,
    const Component* mesh, const Component* material);

/*!
    \brief Render image based on current configuration.
*/
LM_PUBLIC_API void render();

LM_NAMESPACE_END(LM_NAMESPACE)

