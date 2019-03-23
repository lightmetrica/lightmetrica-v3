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

// Initialize master subsystem
LM_PUBLIC_API void init(const std::string& type, const Json& prop);

// Shutdown master subsystem
LM_PUBLIC_API void shutdown();

// Print worker information
LM_PUBLIC_API void printWorkerInfo();

// Execute rendering
LM_PUBLIC_API void render();

// Register a callback function to be called when a task is finished
using WorkerTaskFinishedFunc = std::function<void(long long processed)>;
LM_PUBLIC_API void onWorkerTaskFinished(const WorkerTaskFinishedFunc& func);

// Process a worker task
LM_PUBLIC_API void processWorkerTask(long long start, long long end);

// Notify process has completed to workers
LM_PUBLIC_API void notifyProcessCompleted();

class NetMasterContext : public Component {
public:
    virtual void printWorkerInfo() = 0;
    virtual void render() = 0;
    virtual void onWorkerTaskFinished(const WorkerTaskFinishedFunc& func) = 0;
    virtual void processWorkerTask(long long start, long long end) = 0;
    virtual void notifyProcessCompleted() = 0;
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

// Initialize worker subsystem
LM_PUBLIC_API void init(const std::string& type, const Json& prop);

// Shutdown worker subsystem
LM_PUBLIC_API void shutdown();

// Run event loop
LM_PUBLIC_API void run();

// Register a callback function to be called when all processes have completed.
using ProcessCompletedFunc = std::function<void()>;
LM_PUBLIC_API void onProcessCompleted(const ProcessCompletedFunc& func);

// Register a callback function to process a task
using NetWorkerProcessFunc = std::function<void(long long start, long long end)>;
LM_PUBLIC_API void foreach(const NetWorkerProcessFunc& process);

class NetWorkerContext : public Component {
public:
    virtual void run() = 0;
    virtual void onProcessCompleted(const ProcessCompletedFunc& func) = 0;
    virtual void foreach(const NetWorkerProcessFunc& process) = 0;
};

/*!
    @}
*/

LM_NAMESPACE_END(worker)

// ----------------------------------------------------------------------------

LM_NAMESPACE_END(net)
LM_NAMESPACE_END(LM_NAMESPACE)
