/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "component.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)
LM_NAMESPACE_BEGIN(distributed)

// ------------------------------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(master)

/*!
    \addtogroup distributed
    @{
*/

/*!
    \brief Initialize master context.
    \param prop Configuration properties.
*/
LM_PUBLIC_API void init(const Json& prop);

/*!
    \brief Shutdown master context.
*/
LM_PUBLIC_API void shutdown();

/*!
    \brief Print worker information.

    \rst
    This function prints currently connected workers using the log output.
    \endrst
*/
LM_PUBLIC_API void print_worker_info();

/*!
    \brief Allow or disallow new connections by workers.
    \param allow True to allow connection.

    \rst
    If ``allow = false``, this function disables new connection by workers.
    The default state is ``allow = true``.
    \endrst
*/
LM_PUBLIC_API void allow_worker_connection(bool allow);

/*!
    \brief Synchronize the internal state with the workers.
*/
LM_PUBLIC_API void sync();

/*!
    \brief Callback functon to be called when the task is finished.
    \param processed Processed number of samples.
*/
using WorkerTaskFinishedFunc = std::function<void(long long processed)>;

/*!
    \brief Register a callback function to be called when a task is finished.
    \param func Callback function.
*/
LM_PUBLIC_API void on_worker_task_finished(const WorkerTaskFinishedFunc& func);

/*!
    \brief Process a worker task.
    \param start Lower bound of the sample range. Inclusive.
    \param end Upper bound of the samlpe range. Exclusive.

    \rst
    This function notifies to process the tasks
    in the sample range of ``[start, end)``.
    \endrst
*/
LM_PUBLIC_API void process_worker_task(long long start, long long end);

/*!
    \brief Notify process has completed to workers.
*/
LM_PUBLIC_API void notify_process_completed();

/*!
    \brief Gather films from workers.
    \param filmloc Locator of the film asset.
*/
LM_PUBLIC_API void gather_film(const std::string& filmloc);

/*!
    @}
*/

LM_NAMESPACE_END(master)

// ------------------------------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(worker)

/*!
    \addtogroup distributed
    @{
*/

/*!
    \brief Initialize worker context.
    \param prop Configuration properties.
*/
LM_PUBLIC_API void init(const Json& prop);

/*!
    \brief Shutdown worker context.
*/
LM_PUBLIC_API void shutdown();

/*!
    \brief Run event loop.
*/
LM_PUBLIC_API void run();

/*!
    \brief Callback function being called when all processes have completed.
*/
using ProcessCompletedFunc = std::function<void()>;

/*!
    \brief Register a callback function to be called when all processes have completed.
    \param func Callback function
*/
LM_PUBLIC_API void on_process_completed(const ProcessCompletedFunc& func);

/*!
    \brief Callback function to process a task.
    \param start Lower bound of the sample range. Inclusive.
    \param end Upper bound of the samlpe range. Exclusive.
*/
using WorkerProcessFunc = std::function<void(long long start, long long end)>;

/*!
    \brief Register a callback function to process a task.
    \param process Callback function.
*/
LM_PUBLIC_API void foreach(const WorkerProcessFunc& process);

/*!
    @}
*/

LM_NAMESPACE_END(worker)

// ------------------------------------------------------------------------------------------------

LM_NAMESPACE_END(distributed)
LM_NAMESPACE_END(LM_NAMESPACE)
