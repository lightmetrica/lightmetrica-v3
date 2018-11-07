/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "parallel.h"
#include "component.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)
LM_NAMESPACE_BEGIN(parallel)
LM_NAMESPACE_BEGIN(detail)

class ParallelContext : public Component {
public:
    virtual int numThreads() const = 0;
    virtual void foreach(long long numSamples, const ParallelProcessFunc& processFunc) const = 0;
};

LM_NAMESPACE_END(detail)
LM_NAMESPACE_END(parallel)
LM_NAMESPACE_END(LM_NAMESPACE)
