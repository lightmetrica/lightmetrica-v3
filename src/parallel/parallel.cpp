/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/parallel.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE::parallel)

using Instance = comp::detail::ContextInstance<detail::ParallelContext>;

LM_PUBLIC_API void init(const std::string& type, const Json& prop) {
    Instance::init(type, prop);
}

LM_PUBLIC_API void shutdown() {
    Instance::shutdown();
}

LM_PUBLIC_API int numThreads() {
    return Instance::get().numThreads();
}

LM_PUBLIC_API bool mainThread() {
    return Instance::get().mainThread();
}

LM_PUBLIC_API void foreach(long long numSamples, const ParallelProcessFunc& processFunc) {
    Instance::get().foreach(numSamples, processFunc);
}

LM_NAMESPACE_END(LM_NAMESPACE::parallel)
