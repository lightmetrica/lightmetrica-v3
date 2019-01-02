/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "component.h"
#include <fmt/format.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)
LM_NAMESPACE_BEGIN(log)

// ----------------------------------------------------------------------------

/*!
    \addtogroup log
    @{
*/

//! Default logger type
constexpr const char* DefaultType = "logger::default";

/*!
    \brief Log level.

    \rst
    Log messages have their own importance levels.
    When you want to categorize the log messages according to the importance,
    you can use convenience macros to generate messages with corresponding 
    importance levels. For instance, :func:`LM_ERROR` macro generates a message
    with ``Err`` log level.
    \endrst
*/
enum class LogLevel {
    /*!
        \rst
        Debug message.
        You may use this level to specify the error messages
        are only emitted in a Debug session.
        You can generate a log message with this type by :func:`LM_DEBUG` macro.
        \endrst
    */
    Debug,
    /*!
        \rst
        Information message.
        You may use this level to notice something to the user. 
        Typical usage is to indicate the execution flow of the application
        before/after the execution enters/leaves the codes of heavy computation or IO.
        You can generate a log message with this type by :func:`LM_INFO` macro.
        \endrst
    */
    Info,
    
    Warn,           //!< Warning
    Err,            //!< Error
    Progress,       //!< Progress message
    ProgressEnd,    //!< End of progress
};

/*!
    \brief Initialize logger context.
*/
LM_PUBLIC_API void init(const std::string& type = DefaultType, const Json& prop = {});

/*!
    \brief Shutdown logger context.
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
    ScopedInit(const std::string& type = DefaultType, const Json& prop = {}) { init(type, prop); }
    ~ScopedInit() { shutdown(); }
    LM_DISABLE_COPY_AND_MOVE(ScopedInit)
};

/*!
    @}
*/

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(detail)

/*!
    \addtogroup log
    @{
*/

/*!
    \brief Logger context.
*/
class LoggerContext : public Component {
public:
    virtual void log(LogLevel level, const char* filename, int line, const char* message) = 0;
    virtual void updateIndentation(int n) = 0;
};

/*!
    @}
*/

LM_NAMESPACE_END(detail)

// ----------------------------------------------------------------------------

LM_NAMESPACE_END(log)
LM_NAMESPACE_END(LM_NAMESPACE)

// ----------------------------------------------------------------------------

/*!
    \addtogroup log
    @{
*/

/*!
    \brief Log error message.
*/
#if LM_COMPILER_MSVC
#define LM_ERROR(message, ...) LM_NAMESPACE::log::log( \
    LM_NAMESPACE::log::LogLevel::Err, __FILE__, __LINE__, message, __VA_ARGS__)
#else
#define LM_ERROR(message, ...) LM_NAMESPACE::log::log( \
    LM_NAMESPACE::log::LogLevel::Err, __FILE__, __LINE__, message, ## __VA_ARGS__)
#endif

/*!
    \brief Log warning message.
*/
#if LM_COMPILER_MSVC
#define LM_WARN(message, ...) LM_NAMESPACE::log::log( \
    LM_NAMESPACE::log::LogLevel::Warn, __FILE__, __LINE__, message, __VA_ARGS__)
#else
#define LM_WARN(message, ...) LM_NAMESPACE::log::log( \
    LM_NAMESPACE::log::LogLevel::Warn, __FILE__, __LINE__, message, ## __VA_ARGS__)
#endif

/*!
    \brief Log info message.
*/
#if LM_COMPILER_MSVC
#define LM_INFO(message, ...) LM_NAMESPACE::log::log( \
    LM_NAMESPACE::log::LogLevel::Info, __FILE__, __LINE__, message, __VA_ARGS__)
#else
#define LM_INFO(message, ...) LM_NAMESPACE::log::log( \
    LM_NAMESPACE::log::LogLevel::Info, __FILE__, __LINE__, message, ## __VA_ARGS__)
#endif

/*!
    \brief Log debug message.
*/
#if LM_COMPILER_MSVC
#define LM_DEBUG(message, ...) LM_NAMESPACE::log::log( \
    LM_NAMESPACE::log::LogLevel::Debug, __FILE__, __LINE__, message, __VA_ARGS__)
#else
#define LM_DEBUG(message, ...) LM_NAMESPACE::log::log( \
    LM_NAMESPACE::log::LogLevel::Debug, __FILE__, __LINE__, message, ## __VA_ARGS__)
#endif

/*!
    \brief Log progress outputs.
*/
#if LM_COMPILER_MSVC
#define LM_PROGRESS(message, ...) LM_NAMESPACE::log::log( \
    LM_NAMESPACE::log::LogLevel::Progress, __FILE__, __LINE__, message, __VA_ARGS__)
#else
#define LM_PROGRESS(message, ...) LM_NAMESPACE::log::log( \
    LM_NAMESPACE::log::LogLevel::Progress, __FILE__, __LINE__, message, ## __VA_ARGS__)
#endif

/*!
    \brief Log end of progress outputs.
*/
#if LM_COMPILER_MSVC
#define LM_PROGRESS_END(message, ...) LM_NAMESPACE::log::log( \
    LM_NAMESPACE::log::LogLevel::ProgressEnd, __FILE__, __LINE__, message, __VA_ARGS__)
#else
#define LM_PROGRESS_END(message, ...) LM_NAMESPACE::log::log( \
    LM_NAMESPACE::log::LogLevel::ProgressEnd, __FILE__, __LINE__, message, ## __VA_ARGS__)
#endif

/*!
    \brief Adds an indentation in the current scope.
*/
#define LM_INDENT() LM_NAMESPACE::log::LogIndenter \
    LM_TOKENPASTE2(logIndenter_, __LINE__)

/*!
    @}
*/