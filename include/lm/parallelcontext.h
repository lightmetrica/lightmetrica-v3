/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "parallel.h"
#include "component.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)
LM_NAMESPACE_BEGIN(parallel)

/*!
    \addtogroup parallel
    @{
*/

/*!
    \brief Parallel context.

    \rst
    You may implement this interface to implement user-specific parallel subsystem.
    Each virtual function corresponds to API call with a free function
    inside ``parallel`` namespace.
    \endrst
*/
class ParallelContext : public Component {
public:
    virtual int num_threads() const = 0;
    virtual bool main_thread() const = 0;
    virtual void foreach(long long numSamples, const ParallelProcessFunc& processFunc, const ProgressUpdateFunc& progressFunc) const = 0;
};

/*!
    @}
*/

LM_NAMESPACE_END(parallel)
LM_NAMESPACE_END(LM_NAMESPACE)
