/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "component.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)
LM_NAMESPACE_BEGIN(net)

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(master)

/*!
    \addtogroup net
    @{
*/

// Initialize master subsystem.
LM_PUBLIC_API void init(const std::string& type, const Json& prop);

// Shutdown master subsystem.
LM_PUBLIC_API void shutdown();

// Monitor socket
LM_PUBLIC_API void monitor();

// Add a worker
LM_PUBLIC_API void addWorker(const std::string& address, int port);

// Print worker information
LM_PUBLIC_API void printWorkerInfo();

// Execute rendering.
LM_PUBLIC_API void render();

class NetMasterContext : public Component {
public:
    virtual void monitor() = 0;
    virtual void addWorker(const std::string& address, int port) = 0;
    virtual void printWorkerInfo() = 0;
    virtual void render() = 0;
};

/*!
    @}
*/

LM_NAMESPACE_END(master)

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(worker)

/*!
    \addtogroup net
    @{
*/

// Initialize worker subsystem.
LM_PUBLIC_API void init(const std::string& type, const Json& prop);

// Shutdown worker subsystem.
LM_PUBLIC_API void shutdown();

// Run event loop.
LM_PUBLIC_API void run();

class NetWorkerContext : public Component {
public:
    virtual void run() = 0;
};

/*!
    @}
*/

LM_NAMESPACE_END(worker)

// ----------------------------------------------------------------------------

LM_NAMESPACE_END(net)
LM_NAMESPACE_END(LM_NAMESPACE)
