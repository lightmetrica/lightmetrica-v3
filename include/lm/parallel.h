/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "common.h"
#include "jsontype.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)
LM_NAMESPACE_BEGIN(parallel)

/*!
    \addtogroup parallel
    @{
*/

//! Default parallel context type
constexpr const char* DefaultType = "openmp";

/*!
    \brief Initialize parallel context.
    \param type Type of parallel subsystem.
    \param prop Properties for configuration.

    \rst
    This function initializes the logger subsystem specified by logger type ``type``.
    The function is implicitly called by the framework
    so the user do not want to explicitly call this function.
    \endrst
*/
LM_PUBLIC_API void init(const std::string& type = DefaultType, const Json& prop = {});

/*!
    \brief Shutdown parallel context.
    
    \rst
    This function shutdowns the parallell subsystem.
    You do not want to call this function because
    it is called implicitly by the framework.
    \endrst
*/
LM_PUBLIC_API void shutdown();

/*!
    \brief Get number of threads configured for the subsystem.
    \return Number of threads.
*/
LM_PUBLIC_API int num_threads();

/*!
    \brief Check if current thread is the main thread.
    \return `true` if the current thread is the main thread, `false` otherwise.
*/
LM_PUBLIC_API bool main_thread();

/*!
    \brief Callback function for parallel process.
    \param index Index of iteration.
    \param threadId Thread identifier in `0 ... num_threads()-1`.
*/
using ParallelProcessFunc = std::function<void(long long index, int threadid)>;

/*!
    \brief Callback function for progress updates.
    \param processed Processed number of samples.
*/
using ProgressUpdateFunc = std::function<void(long long processed)>;

/*!
    \brief Parallel for loop.
    \param num_samples Total number of samples.
    \param process_func Callback function called for each iteration.
    \param progress_func Callback function called for each progress update.

    \rst
    We provide an abstraction for the parallel loop specifialized for rendering purpose.
    \endrst
*/
LM_PUBLIC_API void foreach(long long num_samples, const ParallelProcessFunc& process_func, const ProgressUpdateFunc& progress_func);

/*!
    \brief Parallel for loop.
    \param num_samples Total number of samples.
    \param process_func Callback function called for each iteration.
*/
LM_INLINE void foreach(long long num_samples, const ParallelProcessFunc& process_func) {
    foreach(num_samples, process_func, [](long long) {});
}

/*!
    @}
*/

LM_NAMESPACE_END(parallel)
LM_NAMESPACE_END(LM_NAMESPACE)
