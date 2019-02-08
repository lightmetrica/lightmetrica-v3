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
    \brief Query drawing to visual debugger.
*/
LM_PUBLIC_API void draw(int type, const std::vector<Vec3>& vs);

/*!
    \brief Debugio base context.
*/
class DebugioContext : public Component {
public:
    virtual void handleMessage(const std::string& message) { LM_UNUSED(message); }
    virtual void syncUserContext() {}
    virtual void draw(int type, const std::vector<Vec3>& vs) { LM_UNUSED(type, vs); }
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
    \brief Start to run the server.
*/
LM_PUBLIC_API void run();

using HandleMessageFunc = std::function<void(const std::string& message)>;
LM_PUBLIC_API void on_handleMessage(const HandleMessageFunc& process);

using SyncUserContextFunc = std::function<void()>;
LM_PUBLIC_API void on_syncUserContext(const SyncUserContextFunc& process);

using DrawFunc = std::function<void(int type, const std::vector<Vec3>& vs)>;
LM_PUBLIC_API void on_draw(const DrawFunc& process);

class DebugioServerContext : public Component {
public:
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
