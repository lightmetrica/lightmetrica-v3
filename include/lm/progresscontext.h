/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "progress.h"
#include "component.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)
LM_NAMESPACE_BEGIN(progress)

/*!
    \addtogroup progress
    @{
*/

/*!
    \brief Progress context.

    \rst
    You may implement this interface to implement user-specific progress reporting subsystem.
    Each virtual function corresponds to API call with a free function
    inside ``progress`` namespace.
    \endrst
*/
class ProgressContext : public Component {
public:
    virtual void start(ProgressMode mode, long long total, double totalTime) = 0;
    virtual void update(long long processed) = 0;
    virtual void updateTime(Float elapsed) = 0;
    virtual void end() = 0;
};

/*!
    @}
*/

LM_NAMESPACE_END(progress)
LM_NAMESPACE_END(LM_NAMESPACE)
