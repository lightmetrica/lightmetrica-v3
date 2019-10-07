/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "component.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)
LM_NAMESPACE_BEGIN(distributed)

// ----------------------------------------------------------------------------

/*!
    \addtogroup dist
    @{
*/

// Initialize master subsystem
LM_PUBLIC_API void init(const Json& prop);

// Shutdown master subsystem
LM_PUBLIC_API void shutdown();

// Print worker information
LM_PUBLIC_API void printWorkerInfo();

// Allow or disallow new connections by workers
LM_PUBLIC_API void allowWorkerConnection(bool allow);

// Synchronize the internal state with the workers
LM_PUBLIC_API void sync();

// Register a callback function to be called when a task is finished
using WorkerTaskFinishedFunc = std::function<void(long long processed)>;
LM_PUBLIC_API void onWorkerTaskFinished(const WorkerTaskFinishedFunc& func);

// Process a worker task
LM_PUBLIC_API void processWorkerTask(long long start, long long end);

// Notify process has completed to workers
LM_PUBLIC_API void notifyProcessCompleted();

// Gather films from workers
LM_PUBLIC_API void gatherFilm(const std::string& filmloc);

/*!
    @}
*/

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(worker)

/*!
    \addtogroup dist
    @{
*/

// Initialize worker subsystem
LM_PUBLIC_API void init(const Json& prop);

// Shutdown worker subsystem
LM_PUBLIC_API void shutdown();

// Run event loop
LM_PUBLIC_API void run();

// Register a callback function to be called when all processes have completed.
using ProcessCompletedFunc = std::function<void()>;
LM_PUBLIC_API void onProcessCompleted(const ProcessCompletedFunc& func);

// Register a callback function to process a task
using WorkerProcessFunc = std::function<void(long long start, long long end)>;
LM_PUBLIC_API void foreach(const WorkerProcessFunc& process);

/*!
    @}
*/

LM_NAMESPACE_END(worker)

// ----------------------------------------------------------------------------

LM_NAMESPACE_END(distributed)
LM_NAMESPACE_END(LM_NAMESPACE)
