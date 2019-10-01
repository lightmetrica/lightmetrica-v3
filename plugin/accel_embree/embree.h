/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include <lm/logger.h>
#pragma warning(push)
#pragma warning(disable:4324)   // structure was padded due to alignment specifier
#include <embree3/rtcore.h>
#pragma warning(pop)

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

static void handleEmbreeError(void*, RTCError code, const char* str = nullptr) {
    if (code == RTC_ERROR_NONE) {
        return;
    }

    std::string codestr;
    switch (code) {
        case RTC_ERROR_UNKNOWN:           { codestr = "RTC_ERROR_UNKNOWN"; break; }
        case RTC_ERROR_INVALID_ARGUMENT:  { codestr = "RTC_ERROR_INVALID_ARGUMENT"; break; }
        case RTC_ERROR_INVALID_OPERATION: { codestr = "RTC_ERROR_INVALID_OPERATION"; break; }
        case RTC_ERROR_OUT_OF_MEMORY:     { codestr = "RTC_ERROR_OUT_OF_MEMORY"; break; }
        case RTC_ERROR_UNSUPPORTED_CPU:   { codestr = "RTC_ERROR_UNSUPPORTED_CPU"; break; }
        case RTC_ERROR_CANCELLED:         { codestr = "RTC_ERROR_CANCELLED"; break; }
        default:                          { codestr = "Invalid error code"; break; }
    }

    LM_ERROR("Embree error [code='{}']", codestr);
    if (str) {
        LM_INDENT();
        LM_ERROR(str);
    }

    throw std::runtime_error(codestr);
}

LM_NAMESPACE_END(LM_NAMESPACE)
