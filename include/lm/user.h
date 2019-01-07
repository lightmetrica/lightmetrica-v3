/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "component.h"
#include "math.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \addtogroup user
    @{
*/

// ----------------------------------------------------------------------------

/*!
    \brief Initialize the renderer.
    \param prop Properties for configuration.
    \see `example/blank.cpp`

    \rst
    The framework must be initialized with this function before any use of other APIs.
    The properties are passed as JSON format and used to initialize
    the internal subsystems of the framework.
    \endrst
*/
LM_PUBLIC_API void init(const Json& prop = {});

/*!
    \brief Shutdown the renderer.

    \rst
    You don't need to explicitly call this function at the end of your application.
    You want to call this function if you want explicit shutdown of the renderer.
    If you want to use scoped initialization/shutdown,
    consider to use :class:`lm::ScopedInit`.
    \endrst
*/
LM_PUBLIC_API void shutdown();

/*!
    \brief Create an asset.
    \param name Identifier of the asset.
    \param implKey Name of the asset to create.
    \param prop Properties for configuration.
    \see `example/blank.cpp`

    \rst
    Asset represents a basic component of the scene object such as meshes or materials.
    We use assets as building blocks to construct the scene of the framework.
    This function creates an instance of an asset and register it to the framework
    by a given identifier. ``name`` is used as a reference by other APIs.
    ``implKey`` has a format of ``<asset type>::<implementation>``.
    If ``name`` is already registered or ``implKey`` is missing,
    the framework will generate corresponding error messages
    through logger system as well as runtime error exception.
    \endrst
*/
LM_PUBLIC_API void asset(const std::string& name,
    const std::string& implKey, const Json& prop);

/*!
    \brief Get an asset by name.
    \param name Identifier of the asset.
    \return Pointer to the registered asset. ``nullptr`` if not registered.

    \rst
    If the asset specified by ``name`` is not registered,
    the function will generate an error on logger and return nullptr.
    \endrst
*/
LM_PUBLIC_API Component* getAsset(const std::string& name);

/*!
    \brief Create a primitive and add it to the scene.
    \param transform Transformation matrix.
    \param prop Properties for configuration.
    \see `example/quad.cpp`

    \rst
    This function creates a primitive and registers to the framework.
    A primitive is a scene object associating the assets such as
    meshes or materials. The coordinates of the object is
    speficied by a 4x4 transformation matrix.
    We can use the same assets to define different primitives
    with different transformations.
    \endrst
*/
LM_PUBLIC_API void primitive(Mat4 transform, const Json& prop);

/*!
    \brief Create primitives from a model.
    \param transform Transformation matrix.
    \param modelName Identifier of `model` asset.
    \see `example/raycast.cpp`

    \rst
    A ``model`` asset internally creates a set of meshes and materials.
    This function generates a set of primitives from a model asset.
    The transformation is applied to all primitives to be generated.
    \endrst
*/
LM_PUBLIC_API void primitives(Mat4 transform, const std::string& modelName);

/*!
    \brief Build acceleration structure.
    \param accelName Type of acceleration structure.
    \see `example/raycast.cpp`
    
    \rst
    Some renderers require acceleration structure for ray-scene intersections.
    This function internally creates and registers an acceleration structure
    used by other parts of the framework.
    You may specify the acceleration structure type by ``accel::<type>`` format.
    \endrst
*/
LM_PUBLIC_API void build(const std::string& accelName);

/*!
    \brief Render image based on current configuration.
    \param rendererName Type of renderer.
    \param prop Property for configuration.
    \see `example/raycast.cpp`

    \rst
    Once you have completed all configuration of the scene,
    you may start rendering using this function.
    You may specify the renderer type by ``renderer::<type>`` format.
    \endrst
*/
LM_PUBLIC_API void render(const std::string& rendererName, const Json& prop);

/*!
    \brief Save an image.
    \param filmName Identifier of a film asset.
    \param outpath Output path.
    \see `example/blank.cpp`

    \rst
    This function saves the film to the specified output path.
    The behavior of the save depends on the asset type specified in ``filmName``.
    \endrst
*/
LM_PUBLIC_API void save(const std::string& filmName, const std::string& outpath);

/*!
    \brief Get buffer of an image.
    \param filmName Identifier of a film asset.
    \return Film buffer.

    \rst
    This function obtains the buffer to the film asset specified by ``filmName``.
    \endrst
*/
LM_PUBLIC_API FilmBuffer buffer(const std::string& filmName);

/*!
    \brief Serialize the internal state of the framework to a file.
    \param path Output file path.
*/
LM_PUBLIC_API void serialize(const std::string& path);

/*!
    \brief Deserialize the internal state of the framework from a file.
    \param path Input file path.
*/
LM_PUBLIC_API void deserialize(const std::string& path);

// ----------------------------------------------------------------------------

/*!
    \brief Scoped guard of `init` and `shutdown` functions.
    \rst
    Example:

    .. code-block:: cpp

       {
            ScopedInit init_;
            // Do something using API
            // ...
       }
       // Now the framework was safely shutdown.
       // All API calls after this line generates errors.
    \endrst
*/
class ScopedInit {
public:
    ScopedInit() { init(); }
    ~ScopedInit() { shutdown(); }
    LM_DISABLE_COPY_AND_MOVE(ScopedInit)
};

/*!
    @}
*/

LM_NAMESPACE_END(LM_NAMESPACE)
