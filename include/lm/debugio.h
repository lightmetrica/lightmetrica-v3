/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "component.h"

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
    \brief Start to run the server.
*/
LM_PUBLIC_API void run();

/*!
    \brief Handle a message.
*/
LM_PUBLIC_API void handleMessage(const std::string& message);

// ----------------------------------------------------------------------------

/*!
    \brief Debugio base context.
*/
class DebugioContext : public Component {
public:
    virtual void handleMessage(const std::string& message) = 0;
};

/*!
    \brief Debugio server context.
*/
class DebugioServerContext : public DebugioContext {
public:
    virtual void run() = 0;
};

/*!
    @}
*/

LM_NAMESPACE_END(debugio)
LM_NAMESPACE_END(LM_NAMESPACE)
