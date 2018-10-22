/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "common.h"
#include <functional>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)
LM_NAMESPACE_BEGIN(parallel)

using ParallelProcessFunc = std::function<void(long long index, int threadId, bool init)>;
LM_PUBLIC_API void process(const ParallelProcessFunc& processFunc);

LM_NAMESPACE_END(parallel)
LM_NAMESPACE_END(LM_NAMESPACE)
