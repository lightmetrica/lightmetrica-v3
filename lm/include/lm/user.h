/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "component.h"
#include "math.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// ----------------------------------------------------------------------------

/*!
    \brief Initializes the renderer.
*/
LM_PUBLIC_API void init(const Json& prop = {});

/*!
    \brief Shutdown the renderer.
*/
LM_PUBLIC_API void shutdown();

/*!
    \brief Create an asset.
*/
LM_PUBLIC_API void asset(const std::string& name,
    const std::string& implKey, const Json& prop);

/*!
    \brief Get an asset by name.
*/
LM_PUBLIC_API Component* getAsset(const std::string& name);

/*!
    \brief Create a primitive and add it to the scene.
*/
LM_PUBLIC_API void primitive(Mat4 transform, const Json& prop);

/*!
    \brief Create primitives from a model.
*/
LM_PUBLIC_API void primitives(Mat4 transform, const std::string& modelName);

/*!
    \brief Render image based on current configuration.
*/
LM_PUBLIC_API void render(const std::string& rendererName,
    const std::string& accelName, const Json& prop);

/*!
    \brief Save an image.
*/
LM_PUBLIC_API void save(const std::string& filmName, const std::string& outpath);

/*!
    \brief Get buffer of an image.
*/
LM_PUBLIC_API FilmBuffer buffer(const std::string& filmName);

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(detail)

/*!
    \brief Scoped guard of `init` and `shutdown` functions.
*/
class ScopedInit {
public:
    ScopedInit() { init(); }
    ~ScopedInit() { shutdown(); }
    LM_DISABLE_COPY_AND_MOVE(ScopedInit)
};

LM_NAMESPACE_END(detail)
LM_NAMESPACE_END(LM_NAMESPACE)
