/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "component.h"
#include "math.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)
LM_NAMESPACE_BEGIN(debugio)
LM_NAMESPACE_BEGIN(client)

/*!
    \addtogroup debugio
    @{
*/

/*!
    \brief Initialize debugio context.
    \param prop Configuration properties.

    \rst
    This function initializes debugio subsystem with specified type and properties.
    \endrst
*/
LM_PUBLIC_API void init(const Json& prop);

/*!
    \brief Shutdown debugio context.
    
    \rst
    This function shutdowns debugio subsystem.
    You may consider to use :cpp:class:`lm::debugio::ScopedInit` class if you want to explicitly shutdown
    the subsystem at the end of the scope, instead of call this function directly.
    \endrst
*/
LM_PUBLIC_API void shutdown();

/*!
    \brief Handle a message.
    \param message Send a message to the server.
*/
LM_PUBLIC_API void handle_message(const std::string& message);

/*!
    \brief Synchronize the state of user context.

    \rst
    This function synchronizes the internal state of the user context
    of the client process to the server process.
    \endrst
*/
LM_PUBLIC_API void sync_user_context();

/*!
    \brief Debugio mesh type.

    \rst
    This structure represents mesh type for debug visualization,
    used as an argument of :cpp:func:`lm::debugio::draw` function.
    \endrst
*/
enum {
    Triangles = 1<<0,   //!< Triangle mesh.
    LineStrip = 1<<1,   //!< Line strip.
    Lines     = 1<<2,   //!< Lines.
    Points    = 1<<3,   //!< Points.
};

/*!
    \brief Query drawing to visual debugger.
    \param type Mesh type.
    \param color Mesh color.
    \param vs Mesh vertices.
*/
LM_PUBLIC_API void draw(int type, Vec3 color, const std::vector<Vec3>& vs);

/*!
    \brief Scoped guard of `init` and `shutdown` functions.
*/
class ScopedInit {
public:
    ScopedInit(const Json& prop) { init(prop); }
    ~ScopedInit() { shutdown(); }
    LM_DISABLE_COPY_AND_MOVE(ScopedInit)
};

/*!
    @}
*/

LM_NAMESPACE_END(client)

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(server)

/*!
    \addtogroup debugio
    @{
*/

/*!
    \brief Initialize debugio server context.
    \param prop Configuration properties.

    \rst
    This function initializes debugio server subsystem with specified type and properties.
    \endrst
*/
LM_PUBLIC_API void init(const Json& prop);

/*!
    \brief Shutdown debugio server context.

    \rst
    This function shutdowns debugio server subsystem.
    You may consider to use :cpp:class:`lm::debugio::server::ScopedInit` class if you want to explicitly shutdown
    the subsystem at the end of the scope, instead of call this function directly.
    \endrst
*/
LM_PUBLIC_API void shutdown();

/*!
    \brief Poll events.
*/
LM_PUBLIC_API void poll();

/*!
    \brief Start to run the server.
*/
LM_PUBLIC_API void run();

/*!
    \brief Callback function for handle_message.
    \param message Received message.
*/
using HandleMessageFunc = std::function<void(const std::string& message)>;

/*!
    \brief Register callback function for handle_message.
    \param process Callback function.
*/
LM_PUBLIC_API void on_handle_message(const HandleMessageFunc& process);

/*!
    \brief Callback function for sync_user_context.
*/
using SyncUserContextFunc = std::function<void()>;

/*!
    \brief Register callback function for sync_user_context.
    \param process Callback function.
*/
LM_PUBLIC_API void on_sync_user_context(const SyncUserContextFunc& process);

/*!
    \brief Callback function for draw.
    \param type Mesh type.
    \param color Mesh color.
    \param vs Mesh vertices.
*/
using DrawFunc = std::function<void(int type, Vec3 color, const std::vector<Vec3>& vs)>;

/*!
    \brief Register callback function for draw.
    \param process Callback function.
*/
LM_PUBLIC_API void on_draw(const DrawFunc& process);

/*!
    \brief Scoped guard of `init` and `shutdown` functions.
*/
class ScopedInit {
public:
    ScopedInit(const Json& prop) { init(prop); }
    ~ScopedInit() { shutdown(); }
    LM_DISABLE_COPY_AND_MOVE(ScopedInit)
};

/*!
    @}
*/

LM_NAMESPACE_END(server)
LM_NAMESPACE_END(debugio)
LM_NAMESPACE_END(LM_NAMESPACE)
