/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "json.h"
#include <functional>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)
LM_NAMESPACE_BEGIN(parallel)

// ----------------------------------------------------------------------------

//! Default parallel context type
constexpr const char* DefaultType = "parallel::openmp";

/*!
    \brief Explicitly initialize parallel context.
*/
LM_PUBLIC_API void init(const std::string& type = DefaultType, const Json& prop = {});

/*!
    \brief Explicitly shutdown parallel context.
*/
LM_PUBLIC_API void shutdown();

/*!
    \brief Get configured number of threads.
*/
LM_PUBLIC_API int numThreads();

/*!
    \brief Parallel for loop.
*/
using ParallelProcessFunc = std::function<void(long long index, int threadId)>;
LM_PUBLIC_API void foreach(long long numSamples, const ParallelProcessFunc& processFunc);

// ----------------------------------------------------------------------------

LM_NAMESPACE_END(parallel)
LM_NAMESPACE_END(LM_NAMESPACE)
