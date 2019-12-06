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

/*!
    \brief Get underlying collection of assets.
    \return Instance.
*/
LM_PUBLIC_API Assets* assets();

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

/*!
    @}
*/

LM_NAMESPACE_END(LM_NAMESPACE)
