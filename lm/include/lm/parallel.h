/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "detail/component.h"
#include <functional>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)
LM_NAMESPACE_BEGIN(parallel)

/*!
    \brief Explicitly initialize parallel subsystem.
*/
LM_PUBLIC_API void init(const std::string& type, const Json& prop = {});

/*!
    \brief Explicitly shutdown parallel subsystem.
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

LM_NAMESPACE_BEGIN(detail)

class ParallelContext : public Component {
public:
    virtual int numThreads() const = 0;
    virtual void foreach(long long numSamples, const ParallelProcessFunc& processFunc) const = 0;
};

LM_NAMESPACE_END(detail)
LM_NAMESPACE_END(parallel)
LM_NAMESPACE_END(LM_NAMESPACE)
