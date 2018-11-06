/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "../logger.h"
#include "component.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)
LM_NAMESPACE_BEGIN(log)
LM_NAMESPACE_BEGIN(detail)

class LoggerContext : public Component {
public:
    virtual void log(LogLevel level, const char* filename, int line, const char* message) = 0;
    virtual void updateIndentation(int n) = 0;
};

LM_NAMESPACE_END(detail)
LM_NAMESPACE_END(log)
LM_NAMESPACE_END(LM_NAMESPACE)
