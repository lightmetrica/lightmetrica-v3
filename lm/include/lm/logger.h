/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "common.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE::log::detail)

// ----------------------------------------------------------------------------

///! Log level.
enum class LogLevel {
    Trace    = 0,
    Debug    = 1,
    Info     = 2,
    Warn     = 3,
    Err      = 4,
    Critical = 5,
    Off      = 6,
};

///! Initialize logger
LM_PUBLIC_API void init();

///! Shutdown logger
LM_PUBLIC_API void shutdown();

///! Write log message
LM_PUBLIC_API void log(LogLevel level, const char* filename, int line, const char* message);

///! Write log message with formatting
template <typename... Args>
void log(LogLevel level, const char* filename, int line, const char* message, const Args&... args) {
    log(level, fmt::format(message, args).c_str());
}

///! Update indentation
LM_PUBLIC_API void updateIndentation(int n);

///! Log indent contol.
struct LogIndenter {
    LogIndenter()  { updateIndentation(1); }
    ~LogIndenter() { updateIndentation(-1); }
};

// ----------------------------------------------------------------------------

#define LM_LOG_ERROR(message, ...) LM_NAMESPACE::log::detail::log(LM_NAMESPACE::log::detail::LogLevel::Err,   __FILE__, __LINE__, message)
#define LM_LOG_WARN(message, ...)  LM_NAMESPACE::log::detail::log(LM_NAMESPACE::log::detail::LogLevel::Warn,  __FILE__, __LINE__, message)
#define LM_LOG_INFO(message, ...)  LM_NAMESPACE::log::detail::log(LM_NAMESPACE::log::detail::LogLevel::Err,   __FILE__, __LINE__, message)
#define LM_LOG_DEBUG(message, ...) LM_NAMESPACE::log::detail::log(LM_NAMESPACE::log::detail::LogLevel::Debug, __FILE__, __LINE__, message)
#define LM_LOG_INDENTER()          LM_NAMESPACE::logger::detail::LogIndenter LM_TOKENPASTE2(logIndenter_, __LINE__)

// ----------------------------------------------------------------------------

LM_NAMESPACE_END(LM_NAMESPACE::log::detail)
