/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "component.h"
#include "math.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)
LM_NAMESPACE_BEGIN(debugio)

/*!
    \addtogroup debugio
    @{
*/

/*!
    \brief Initialize debugio context.
*/
LM_PUBLIC_API void init(const std::string& type, const Json& prop);

/*!
    \brief Shutdown debugio context.
*/
LM_PUBLIC_API void shutdown();

/*!
    \brief Handle a message.
*/
LM_PUBLIC_API void handleMessage(const std::string& message);

/*!
    \brief Synchronize the state of user context.

    \rst
    This function synchronizes the internal state of the user context
    of the client process to the server process.
    \endrst
*/
LM_PUBLIC_API void syncUserContext();

/*!
    \brief Debugio mesh type.
*/
enum {
    Triangles = 1<<0,
    LineStrip = 1<<1,
    Lines     = 1<<2,
    Points    = 1<<3,
};

/*!
    \brief Query drawing to visual debugger.
*/
LM_PUBLIC_API void draw(int type, Vec3 color, const std::vector<Vec3>& vs);

/*!
    \brief Scoped guard of `init` and `shutdown` functions.
*/
class ScopedInit {
public:
    ScopedInit(const std::string& type, const Json& prop) { init(type, prop); }
    ~ScopedInit() { shutdown(); }
    LM_DISABLE_COPY_AND_MOVE(ScopedInit)
};

/*!
    \brief Debugio context.
*/
class DebugioContext : public Component {
public:
    virtual void handleMessage(const std::string& message) { LM_UNUSED(message); }
    virtual void syncUserContext() {}
    virtual void draw(int type, Vec3 color, const std::vector<Vec3>& vs) { LM_UNUSED(type, color, vs); }
};

/*!
    @}
*/

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(server)

/*!
    \addtogroup debugio
    @{
*/

/*!
    \brief Initialize debugio server context.
*/
LM_PUBLIC_API void init(const std::string& type, const Json& prop);

/*!
    \brief Shutdown debugio server context.
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
    \brief Callback function for handleMessage.
*/
using HandleMessageFunc = std::function<void(const std::string& message)>;

/*!
    \brief Register callback function for handleMessage.
*/
LM_PUBLIC_API void on_handleMessage(const HandleMessageFunc& process);

/*!
    \brief Callback function for syncUserContext.
*/
using SyncUserContextFunc = std::function<void()>;

/*!
    \brief Register callback function for syncUserContext.
*/
LM_PUBLIC_API void on_syncUserContext(const SyncUserContextFunc& process);

/*!
    \brief Callback function for draw.
*/
using DrawFunc = std::function<void(int type, Vec3 color, const std::vector<Vec3>& vs)>;

/*!
    \brief Register callback function for draw.
*/
LM_PUBLIC_API void on_draw(const DrawFunc& process);

/*!
    \brief Scoped guard of `init` and `shutdown` functions.
*/
class ScopedInit {
public:
    ScopedInit(const std::string& type, const Json& prop) { init(type, prop); }
    ~ScopedInit() { shutdown(); }
    LM_DISABLE_COPY_AND_MOVE(ScopedInit)
};

/*!
    \brief Debugio server context.
*/
class DebugioServerContext : public Component {
public:
    virtual void poll() = 0;
    virtual void run() = 0;
    virtual void on_handleMessage(const HandleMessageFunc& process) = 0;
    virtual void on_syncUserContext(const SyncUserContextFunc& process) = 0;
    virtual void on_draw(const DrawFunc& process) = 0;
};

/*!
    @}
*/

LM_NAMESPACE_END(server)
LM_NAMESPACE_END(debugio)
LM_NAMESPACE_END(LM_NAMESPACE)
