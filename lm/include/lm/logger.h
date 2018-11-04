/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "detail/common.h"
#include <fmt/format.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)
LM_NAMESPACE_BEGIN(log)

// ----------------------------------------------------------------------------

/*!
    \brief Log level.
*/
enum class LogLevel {
    Trace    = 0,
    Debug    = 1,
    Info     = 2,
    Warn     = 3,
    Err      = 4,
    Critical = 5,
    Off      = 6,
};

/*!
    \brief Initialize logger subsystem.
*/
LM_PUBLIC_API void init();

/*!
    \brief Shutdown logger subsystem.
*/
LM_PUBLIC_API void shutdown();

/*!
    \brief Write log message.
*/
LM_PUBLIC_API void log(LogLevel level, const char* filename, int line, const char* message);

/*!
    \brief Write log message with formatting.
*/
template <typename... Args>
void log(LogLevel level, const char* filename, int line, const std::string& message, Args&&... args) {
    log(level, filename, line, fmt::format(message, std::forward<Args>(args)...).c_str());
}

/*!
    \brief Update indentation.
*/
LM_PUBLIC_API void updateIndentation(int n);

/*!
    \brief Log indent contol.
*/
struct LogIndenter {
    LogIndenter()  { updateIndentation(1); }
    ~LogIndenter() { updateIndentation(-1); }
};

/*!
    Scoped guard of `init` and `shutdown` functions.
*/
class ScopedInit {
public:
    ScopedInit() { init(); }
    ~ScopedInit() { shutdown(); }
    LM_DISABLE_COPY_AND_MOVE(ScopedInit)
};

// ----------------------------------------------------------------------------

LM_NAMESPACE_END(log)
LM_NAMESPACE_END(LM_NAMESPACE)

// ----------------------------------------------------------------------------

/*!
    \brief Log error message.
*/
#define LM_ERROR(message, ...) LM_NAMESPACE::log::log( \
    LM_NAMESPACE::log::LogLevel::Err, __FILE__, __LINE__, message, __VA_ARGS__)

/*!
    \brief Log warning message.
*/
#define LM_WARN(message, ...) LM_NAMESPACE::log::log( \
    LM_NAMESPACE::log::LogLevel::Warn, __FILE__, __LINE__, message, __VA_ARGS__)

/*!
    \brief Log info message.
*/
#define LM_INFO(message, ...) LM_NAMESPACE::log::log( \
    LM_NAMESPACE::log::LogLevel::Info, __FILE__, __LINE__, message, __VA_ARGS__)

/*!
    \brief Log debug message.
*/
#define LM_DEBUG(message, ...) LM_NAMESPACE::log::log( \
    LM_NAMESPACE::log::LogLevel::Debug, __FILE__, __LINE__, message, __VA_ARGS__)

/*!
    \brief Adds an indentation in the current scope.
*/
#define LM_INDENT() LM_NAMESPACE::log::LogIndenter \
    LM_TOKENPASTE2(logIndenter_, __LINE__)
