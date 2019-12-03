/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "logger.h"
#include "component.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)
LM_NAMESPACE_BEGIN(log)

/*!
    \addtogroup log
    @{
*/

/*!
    \brief Logger context.

    \rst
    You may implement this interface to implement user-specific log subsystem.
    Each virtual function corresponds to API call with functions inside ``log`` namespace.
    \endrst
*/
class LoggerContext : public Component {
public:
    virtual void log(LogLevel level, int severity, const char* filename, int line, const char* message) = 0;
    virtual void update_indentation(int n) = 0;
    virtual void set_severity(int severity) = 0;
};

/*!
    @}
*/

LM_NAMESPACE_END(log)
LM_NAMESPACE_END(LM_NAMESPACE)
