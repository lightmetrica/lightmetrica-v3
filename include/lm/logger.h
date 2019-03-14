/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
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
    importance levels. For instance, :c:func:`LM_ERROR` macro generates a message
    with ``Err`` log level.
    \endrst
*/
enum class LogLevel : int {
    /*!
        \rst
        Debug message.
        You may use this level to specify the error messages
        are only emitted in a Debug session.
        You can generate a log message with this type by :func:`LM_DEBUG` macro.
        \endrst
    */
    Debug = -10,
    /*!
        \rst
        Information message.
        You may use this level to notice information to the user. 
        Typical usage is to indicate the execution flow of the application
        before/after the execution enters/leaves the codes of heavy computation or IO.
        You can generate a log message with this type by :func:`LM_INFO` macro.
        \endrst
    */
    Info = 10,
    /*!
        \rst
        Warning message.
        You may use this level to give warning to the user. 
        Usage might be to convey inconsistent yet continuable state of the execution
        such as handling of default arguments.
        \endrst
    */
    Warn = 20,
    /*!
        \rst
        Error message.
        This error level notifies you an error happens in the execution.
        The error often comes along with immediate shutdown of the renderer.
        \endrst
    */
    Err = 30,
    /*!
        \rst
        Progress message.
        The messages of this log level indicates special message type
        used in the progress update where the message are used specifically
        for the interactive update of the progress report.
        \endrst
    */
    Progress = 100,
    /*!
        \rst
        End of progress message.
        The message indicates special message type
        used in the end of the progress message.
        \endrst
    */
    ProgressEnd = 100,
};

/*!
    \brief Initialize logger context.
    \param type Type of log subsystem.
    \param prop Configuration properties.

    \rst
    This function initializes logger subsystem with specified type and properties.
    \endrst
*/
LM_PUBLIC_API void init(const std::string& type = DefaultType, const Json& prop = {});

/*!
    \brief Shutdown logger context.

    \rst
    This function shutdowns logger subsystem.
    You may consider to use :cpp:class:`lm::log::ScopedInit` class if you want to explicitly shutdown
    the subsystem at the end of the scope, instead of call this function directly.
    \endrst
*/
LM_PUBLIC_API void shutdown();

/*!
    \brief Set severity of the log.
    \param severity Severity.
    
    \rst
    The log messages with severity value larger or equal to the given value will be rendered.
    \endrst
*/
LM_PUBLIC_API void setSeverity(int severity);

inline void setSeverity(LogLevel severity) {
    return setSeverity(int(severity));
}

/*!
    \brief Write log message.
    \param level Log level.
    \param severity Severity.
    \param filename Filename where the log message are generated.
    \param line Line of the code where the log message are generated.
    \param message Log message.

    \rst
    This function posts a log message of specific log level to the logger subsystem.
    The behavior of this function depends on the implementation of the logger.
    You may want to use convenience macros instead of this function
    because the macros automatically extracts filename and line number for you.
    \endrst
*/
LM_PUBLIC_API void log(LogLevel level, int severity, const char* filename, int line, const char* message);

/*!
    \brief Write log message with formatting.
    \param level Log level.
    \param severity Severity.
    \param filename Filename where the log message are generated.
    \param line Line of the code where the log message are generated.
    \param message Log message.
    \param args List of arguments to be replaced according to format in the log message.

    \rst
    This version of the log function posts a log message with formatting.
    The specification follows replacement-based format API by `fmt library`_.

    .. _fmt library: https://github.com/fmtlib/fmt
    \endrst
*/
template <typename... Args>
void log(LogLevel level, int severity, const char* filename, int line, const std::string& message, Args&&... args) {
    if constexpr (sizeof...(Args) == 0) {
        // Avoid application of fmt::format() with no additional argument
        log(level, severity, filename, line, message.c_str());
    }
    else {
        log(level, severity, filename, line, fmt::format(message, std::forward<Args>(args)...).c_str());
    }
}

/*!
    \brief Update indentation.
    \param n Increase / decrease of the indentration.

    \rst
    The log messages can be indented for better visibility.
    This function controls the indentation level by increment or decrement
    of the indentation by an integer. For instance, ``-1`` subtracts one indentation level.
    \endrst
*/
LM_PUBLIC_API void updateIndentation(int n);

/*!
    \brief Log indent contol.

    \rst
    The class controls the indentation level according to the scopes.
    You want to use convenience macro :func:`LM_INDENT` instead of this function.
    \endrst
*/
struct LogIndenter {
    LogIndenter()  { updateIndentation(1); }
    ~LogIndenter() { updateIndentation(-1); }
};

/*!
    \brief Scoped guard of `init` and `shutdown` functions.
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
    
    \rst
    You may implement this interface to implement user-specific log subsystem.
    Each virtual function corresponds to API call with functions inside ``log`` namespace.
    \endrst
*/
class LoggerContext : public Component {
public:
    virtual void log(LogLevel level, int severity, const char* filename, int line, const char* message) = 0;
    virtual void updateIndentation(int n) = 0;
    virtual void setSeverity(int severity) = 0;
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
    \brief Post a log message with a user-defined severity.
    \param severity User-defined severity by integer.
    \param message Log message.
    \param ... Parameters for the format in the log message.
*/
#if LM_COMPILER_MSVC
#define LM_LOG(severity, message, ...) LM_NAMESPACE::log::log( \
    LM_NAMESPACE::log::LogLevel::Info, severity, \
    __FILE__, __LINE__, message, __VA_ARGS__)
#else
#define LM_LOG(severity, message, ...) LM_NAMESPACE::log::log( \
    LM_NAMESPACE::log::LogLevel::Info, severity, \
    __FILE__, __LINE__, message, ## __VA_ARGS__)
#endif

/*!
    \brief Post a log message with error level.
    \param message Log message.
    \param ... Parameters for the format in the log message.
*/
#if LM_COMPILER_MSVC
#define LM_ERROR(message, ...) LM_NAMESPACE::log::log( \
    LM_NAMESPACE::log::LogLevel::Err, int(LM_NAMESPACE::log::LogLevel::Err), \
    __FILE__, __LINE__, message, __VA_ARGS__)
#else
#define LM_ERROR(message, ...) LM_NAMESPACE::log::log( \
    LM_NAMESPACE::log::LogLevel::Err, int(LM_NAMESPACE::log::LogLevel::Err), \
    __FILE__, __LINE__, message, ## __VA_ARGS__)
#endif

/*!
    \brief Post a log message with warning level.
    \param message Log message.
    \param ... Parameters for the format in the log message.
*/
#if LM_COMPILER_MSVC
#define LM_WARN(message, ...) LM_NAMESPACE::log::log( \
    LM_NAMESPACE::log::LogLevel::Warn, int(LM_NAMESPACE::log::LogLevel::Warn), \
    __FILE__, __LINE__, message, __VA_ARGS__)
#else
#define LM_WARN(message, ...) LM_NAMESPACE::log::log( \
    LM_NAMESPACE::log::LogLevel::Warn, int(LM_NAMESPACE::log::LogLevel::Warn), \
    __FILE__, __LINE__, message, ## __VA_ARGS__)
#endif

/*!
    \brief Post a log message with information level.
    \param message Log message.
    \param ... Parameters for the format in the log message.
*/
#if LM_COMPILER_MSVC
#define LM_INFO(message, ...) LM_NAMESPACE::log::log( \
    LM_NAMESPACE::log::LogLevel::Info, int(LM_NAMESPACE::log::LogLevel::Info), \
    __FILE__, __LINE__, message, __VA_ARGS__)
#else
#define LM_INFO(message, ...) LM_NAMESPACE::log::log( \
    LM_NAMESPACE::log::LogLevel::Info, int(LM_NAMESPACE::log::LogLevel::Info), \
    __FILE__, __LINE__, message, ## __VA_ARGS__)
#endif

/*!
    \brief Post a log message with debug level.
    \param message Log message.
    \param ... Parameters for the format in the log message.
*/
#if LM_COMPILER_MSVC
#define LM_DEBUG(message, ...) LM_NAMESPACE::log::log( \
    LM_NAMESPACE::log::LogLevel::Debug, int(LM_NAMESPACE::log::LogLevel::Debug), \
     __FILE__, __LINE__, message, __VA_ARGS__)
#else
#define LM_DEBUG(message, ...) LM_NAMESPACE::log::log( \
    LM_NAMESPACE::log::LogLevel::Debug, int(LM_NAMESPACE::log::LogLevel::Debug), \
    __FILE__, __LINE__, message, ## __VA_ARGS__)
#endif

/*!
    \brief Log progress outputs.
    \param message Log message.
    \param ... Parameters for the format in the log message.
*/
#if LM_COMPILER_MSVC
#define LM_PROGRESS(message, ...) LM_NAMESPACE::log::log( \
    LM_NAMESPACE::log::LogLevel::Progress, int(LM_NAMESPACE::log::LogLevel::Progress), \
    __FILE__, __LINE__, message, __VA_ARGS__)
#else
#define LM_PROGRESS(message, ...) LM_NAMESPACE::log::log( \
    LM_NAMESPACE::log::LogLevel::Progress, int(LM_NAMESPACE::log::LogLevel::Progress), \
    __FILE__, __LINE__, message, ## __VA_ARGS__)
#endif

/*!
    \brief Log end of progress outputs.
    \param message Log message.
    \param ... Parameters for the format in the log message.
*/
#if LM_COMPILER_MSVC
#define LM_PROGRESS_END(message, ...) LM_NAMESPACE::log::log( \
    LM_NAMESPACE::log::LogLevel::ProgressEnd, int(LM_NAMESPACE::log::LogLevel::ProgressEnd), \
    __FILE__, __LINE__, message, __VA_ARGS__)
#else
#define LM_PROGRESS_END(message, ...) LM_NAMESPACE::log::log( \
    LM_NAMESPACE::log::LogLevel::ProgressEnd, int(LM_NAMESPACE::log::LogLevel::ProgressEnd), \
    __FILE__, __LINE__, message, ## __VA_ARGS__)
#endif

/*!
    \brief Adds an indentation in the current scope.
    \rst
    Example:
    
    .. code-block:: cpp

        // Indentation = 0. Produces " message 1"
        LM_INFO("message 1");
        {
            // Indentation = 1. Produces ".. message 2"
            LM_INDENT();
            LM_INFO("message 2");
        }
        // Indentation = 0. Produces " message 3"
        LM_INFO("message 3");
    \endrst
*/
#define LM_INDENT() LM_NAMESPACE::log::LogIndenter \
    LM_TOKENPASTE2(logIndenter_, __LINE__)

/*!
    @}
*/