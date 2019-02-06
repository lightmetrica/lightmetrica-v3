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
    \brief Draw scene.
*/
LM_PUBLIC_API void drawScene();

/*!
    \brief Draw line strip from the vertices.
*/
LM_PUBLIC_API void drawLineStrip(const std::vector<Vec3>& vs);

/*!
    \brief Draw triangles from the vertices.
*/
LM_PUBLIC_API void drawTriangles(const std::vector<Vec3>& vs);

/*!
    \brief Start to run the server.
*/
LM_PUBLIC_API void run();

// ----------------------------------------------------------------------------

/*!
    \brief Debugio base context.
*/
class DebugioContext : public Component {
public:
    virtual void handleMessage(const std::string& message) { LM_UNUSED(message); }
    virtual void syncUserContext() {}
    virtual void drawScene() {}
    virtual void drawLineStrip(const std::vector<Vec3>& vs) { LM_UNUSED(vs); }
    virtual void drawTriangles(const std::vector<Vec3>& vs) { LM_UNUSED(vs); }
};

class DebugioServerContext : public Component {
public:
    virtual void run() = 0;
};

/*!
    @}
*/

LM_NAMESPACE_END(debugio)
LM_NAMESPACE_END(LM_NAMESPACE)
