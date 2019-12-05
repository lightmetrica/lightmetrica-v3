/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "component.h"
#include "math.h"
#include <regex>
#include <fstream>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \addtogroup user
    @{
*/

/*!
    \brief Initialize the renderer.
    \param prop Properties for configuration.
    \see `example/blank.cpp`

    \rst
    The framework must be initialized with this function before any use of other APIs.
    The properties are passed as JSON format and used to initialize
    the internal subsystems of the framework.
    This function initializes some subsystems with default types.
    If you want to configure the subsystem, you want to call each ``init()`` function afterwards.
    \endrst
*/
LM_PUBLIC_API void init(const Json& prop = {});

/*!
    \brief Shutdown the renderer.

    \rst
    Call this function if you want explicit shutdown of the renderer.
    If you want to use scoped initialization/shutdown,
    consider to use :class:`lm::ScopedInit`.
    \endrst
*/
LM_PUBLIC_API void shutdown();

/*!
    \brief Reset the internal state of the framework.

    \rst
    This function resets underlying states including assets and scene to the initial state.
    The global contexts remains same.
    \endrst
*/
LM_PUBLIC_API void reset();

/*!
    \brief Print information about Lightmetrica.
*/
LM_PUBLIC_API void info();

// ------------------------------------------------------------------------------------------------

/*!
    \brief Create asset.
    \param name Identifier of the asset.
    \param impl_key Name of the asset to create.
    \param prop Properties for configuration.
    \return A pointer to the asset.
    \see `example/blank.cpp`

    \rst
    Asset represents a basic component of the scene object such as meshes or materials.
    We use assets as building blocks to construct the scene of the framework.
    This function creates an instance of an asset and register it to the framework
    by a given identifier. ``name`` is used as a reference by other APIs.
    ``impl_key`` has a format of ``<asset type>::<implementation>``.
    If ``name`` is already registered or ``impl_key`` is missing,
    the framework will generate corresponding error messages
    through logger system as well as runtime error exception.
	This function returns a pointer to the instance,
	which is managed internally in the framework.
    \endrst
*/
LM_PUBLIC_API Component* asset(const std::string& name, const std::string& impl_key, const Json& prop);

/*!
	\brief Get the locator of an asset.
	\param name Identifier of the asset.
	\return Locator of the asset.
*/
LM_PUBLIC_API std::string asset(const std::string& name);

// ------------------------------------------------------------------------------------------------

/*!
    \brief Build acceleration structure.
    \param accel_name Type of acceleration structure.
    \param prop Property for configuration.
    \see `example/raycast.cpp`
    
    \rst
    Some renderers require acceleration structure for ray-scene intersections.
    This function internally creates and registers an acceleration structure
    used by other parts of the framework.
    You may specify the acceleration structure type by ``accel::<type>`` format.
    \endrst
*/
LM_PUBLIC_API void build(const std::string& accel_name, const Json& prop = {});

/*!
    \brief Initialize renderer.
    \param renderer_name Type of renderer.
    \param prop Property for configuration.
*/
LM_PUBLIC_API void renderer(const std::string& renderer_name, const Json& prop = {});

/*!
    \brief Render image based on current configuration.
    \param verbose Supress outputs if false.
    \see `example/raycast.cpp`

    \rst
    Once you have completed all configuration of the scene,
    you may start rendering using this function.
    You may specify the renderer type by ``renderer::<type>`` format.
    \endrst
*/
LM_PUBLIC_API void render(bool verbose = true);

/*!
    \brief Initialize renderer and render.
    \param renderer_name Type of renderer.
    \param prop Property for configuration.
*/
static void render(const std::string& renderer_name, const Json& prop = {}) {
    renderer(renderer_name, prop);
    render();
}

// ------------------------------------------------------------------------------------------------

/*!
    \brief Save an image.
    \param film_name Identifier of a film asset.
    \param outpath Output path.
    \see `example/blank.cpp`

    \rst
    This function saves the film to the specified output path.
    The behavior of the save depends on the asset type specified in ``film_name``.
    \endrst
*/
LM_PUBLIC_API void save(const std::string& film_name, const std::string& outpath);

/*!
    \brief Get buffer of an image.
    \param film_name Identifier of a film asset.
    \return Film buffer.

    \rst
    This function obtains the buffer to the film asset specified by ``film_name``.
    \endrst
*/
LM_PUBLIC_API FilmBuffer buffer(const std::string& film_name);

// ------------------------------------------------------------------------------------------------

/*!
    \brief Serialize the internal state of the framework to a stream.
    \param os Output stream.
*/
LM_PUBLIC_API void serialize(std::ostream& os);

/*!
    \brief Deserialize the internal state of the framework from a stream.
    \param is Input stream.
*/
LM_PUBLIC_API void deserialize(std::istream& is);

/*!
    \brief Serialize the internal state to a file.
    \param path Path to a file with serialized state.
*/
LM_INLINE void serialize(const std::string& path) {
    std::ofstream os(path, std::ios::out | std::ios::binary);
    serialize(os);
}

/*!
    \brief Deserialize the internal state to a file.
    \param path to a file with serialized state. 
*/
LM_INLINE void deserialize(const std::string& path) {
    std::ifstream is(path, std::ios::in | std::ios::binary);
    deserialize(is);
}

// ------------------------------------------------------------------------------------------------

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
    ScopedInit(const Json& prop = {}) { init(prop); }
    ~ScopedInit() { shutdown(); }
    LM_DISABLE_COPY_AND_MOVE(ScopedInit)
};

// ------------------------------------------------------------------------------------------------

/*!
    \brief Get index of the root node.
    \return Node index.
*/
LM_PUBLIC_API int root_node();

/*!
    \brief Create primitive node.
    \param prop Properties.
    \return Index of the created node.

    \rst
    TODO: Describe about properties.
    \endrst
*/
LM_PUBLIC_API int primitive_node(const Json& prop);

/*!
    \brief Create group node.
    \return Index of the created node.
*/
LM_PUBLIC_API int group_node();

/*!
    \brief Create instanece group node.
    \return Index of the created node.
*/
LM_PUBLIC_API int instance_group_node();

/*!
    \brief Create transform node.
    \return Index of the created node.
*/
LM_PUBLIC_API int transform_node(Mat4 transform);

/*!
    \brief Add child node.
    \param parent Index of the parent node.
    \param child Index of the node being added to the parent.
*/
LM_PUBLIC_API void add_child(int parent, int child);

/*!
    \brief Add child node from model asset.
    \param parent Index of the parent node.
    \param model_loc Locator of the model asset.
*/
LM_PUBLIC_API void add_child_from_model(int parent, const std::string& model_loc);

/*!
    \brief Create group node from model asset.
    \param model_loc Locator of the model asset.
    \return Index of the created node.
*/
LM_PUBLIC_API int create_group_from_model(const std::string& model_loc);

/*!
    \brief Create primitive(s) and add to the scene.
    \param transform Transformation matrix.
    \param prop Properties for configuration.
    \see `example/quad.cpp`
    \see `example/raycast.cpp`

    \rst
    This function creates primitive(s) and registers to the framework.
    A primitive is a scene object associating the assets such as
    meshes or materials. The coordinates of the object is
    speficied by a 4x4 transformation matrix.
    We can use the same assets to define different primitives
    with different transformations.

    If ``model`` parameter is specified,
    the function will register primitives generated from the model.
    In this case, the transformation is applied to all primitives to be generated.
    \endrst
*/
LM_PUBLIC_API void primitive(Mat4 transform, const Json& prop);

/*!
    @}
*/

// ------------------------------------------------------------------------------------------------

LM_NAMESPACE_END(LM_NAMESPACE)
