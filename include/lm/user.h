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

// ------------------------------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(user)

/*!
    \addtogroup user
    @{
*/

//! Default user type
constexpr const char* DefaultType = "user::default";

/*!
    @}
*/

LM_NAMESPACE_END(user)

// ------------------------------------------------------------------------------------------------

/*!
    \addtogroup user
    @{
*/

/*!
    \brief Initialize the renderer.
    \param type Type of user context.
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
LM_PUBLIC_API void init(const std::string& type = user::DefaultType, const Json& prop = {});

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
    \brief Create an asset.
    \param name Identifier of the asset.
    \param implKey Name of the asset to create.
    \param prop Properties for configuration.
    \return Locator of the asset.
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
LM_PUBLIC_API std::string asset(const std::string& name, const std::string& implKey, const Json& prop);

/*!
    \brief Get the locator of an asset.
    \param name Identifier of the asset.
    \return Locator of the asset.
*/
LM_PUBLIC_API std::string asset(const std::string& name);

// ------------------------------------------------------------------------------------------------

/*!
    \brief Build acceleration structure.
    \param accelName Type of acceleration structure.
    \param prop Property for configuration.
    \see `example/raycast.cpp`
    
    \rst
    Some renderers require acceleration structure for ray-scene intersections.
    This function internally creates and registers an acceleration structure
    used by other parts of the framework.
    You may specify the acceleration structure type by ``accel::<type>`` format.
    \endrst
*/
LM_PUBLIC_API void build(const std::string& accelName, const Json& prop = {});

/*!
    \brief Initialize renderer.
    \param rendererName Type of renderer.
    \param prop Property for configuration.
*/
LM_PUBLIC_API void renderer(const std::string& rendererName, const Json& prop = {});

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
    \param rendererName Type of renderer.
    \param prop Property for configuration.
*/
static void render(const std::string& rendererName, const Json& prop = {}) {
    renderer(rendererName, prop);
    render();
}

// ------------------------------------------------------------------------------------------------

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
    \brief Validate consistency of the component instances.
*/
LM_PUBLIC_API void validate();

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
    ScopedInit(const std::string& type = user::DefaultType, const Json& prop = {}) { init(type, prop); }
    ~ScopedInit() { shutdown(); }
    LM_DISABLE_COPY_AND_MOVE(ScopedInit)
};

// ------------------------------------------------------------------------------------------------

LM_PUBLIC_API int rootNode();
LM_PUBLIC_API int primitiveNode(const Json& prop);
LM_PUBLIC_API int groupNode();
LM_PUBLIC_API int instanceGroupNode();
LM_PUBLIC_API int transformNode(Mat4 transform);
LM_PUBLIC_API void addChild(int parent, int child);
LM_PUBLIC_API void addChildFromModel(int parent, const std::string& modelLoc);
LM_PUBLIC_API int createGroupFromModel(const std::string& modelLoc);

/*!
    \brief Create primitive(s) and add to the scene.
    \param group Group index.
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

LM_NAMESPACE_BEGIN(user)
LM_NAMESPACE_BEGIN(detail)

/*!
    \addtogroup user
    @{
*/

/*!
    \brief User context.

    \rst
    You may implement this interface to replace user APIs.
    Each virtual function corresponds to API call with functions above.
    \endrst
*/
class UserContext : public Component {
public:
    virtual void reset() = 0;
    virtual void info() = 0;
    virtual std::string asset(const std::string& name, const std::string& implKey, const Json& prop) = 0;
    virtual std::string asset(const std::string& name) = 0;
    virtual void build(const std::string& accelName, const Json& prop) = 0;
    virtual void renderer(const std::string& rendererName, const Json& prop) = 0;
    virtual void render(bool verbose) = 0;
    virtual void save(const std::string& filmName, const std::string& outpath) = 0;
    virtual FilmBuffer buffer(const std::string& filmName) = 0;
    virtual void serialize(std::ostream& os) = 0;
    virtual void deserialize(std::istream& is) = 0;
    virtual int rootNode() = 0;
    virtual int primitiveNode(const Json& prop) = 0;
    virtual int groupNode() = 0;
    virtual int instanceGroupNode() = 0;
    virtual int transformNode(Mat4 transform) = 0;
    virtual void addChild(int parent, int child) = 0;
    virtual void addChildFromModel(int parent, const std::string& modelLoc) = 0;
	virtual int createGroupFromModel(const std::string& modelLoc) = 0;
};

/*!
    @}
*/

LM_NAMESPACE_END(detail)
LM_NAMESPACE_END(user)

// ----------------------------------------------------------------------------

LM_NAMESPACE_END(LM_NAMESPACE)
