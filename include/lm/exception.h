/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "common.h"
#include "jsontype.h"
#include <exception>
#include <fmt/format.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)
LM_NAMESPACE_BEGIN(exception)

/*!
    \addtogroup exception
    @{
*/

/*!
    \brief Initialize exception context.
    \param prop Properties for configuration.

    \rst
    This function initializes exception subsystem of the framework.
    The function is implicitly called by the framework
    so the user do not want to explicitly call this function.
    \endrst
*/
LM_PUBLIC_API void init(const Json& prop = {});

/*!
    \brief Shutdown exception context.

    \rst
    This function shutdowns the exception subsystem.
    You do not want to call this function because it is called implicitly by the framework. 
    You may consider to use :class:`ScopedInit` class if you want to explicitly shutdown
    the subsystem at the end of the scope, instead of call this function directly.
    \endrst
*/
LM_PUBLIC_API void shutdown();

/*!
    \brief Enable floating point exceptions.

    \rst
    This function enables floating-point exception.
    You may use :class:`ScopedDisableFPEx` class to automatically enable/disable
    the floating point exception inside a scope.
    \endrst
*/
LM_PUBLIC_API void enableFPEx();

/*!
    \brief Disable floating point exceptions.
    
    \rst
    This function disables floating-point exception.
    You may use :class:`ScopedDisableFPEx` class to automatically enable/disable
    the floating point exception inside a scope.
    \endrst
*/
LM_PUBLIC_API void disableFPEx();

/*!
    \brief Print stack trace.

    \rst
    This function prints a stack trace of the current frame.
    You may find it useful when you want to display additional information
    as well as an error message.
    \endrst
*/
LM_PUBLIC_API void stackTrace();

/*!
    \brief Scoped disable of floating point exception.

    \rst
    A convenience class when you want to temporarily disable floating point exceptions.
    This class is useful when you want to use some external libraries that
    does not check strict floating point exceptions. 
    
    Example:

    .. code-block:: cpp

       // Floating-point exception is enabled
       enableFPEx();
       {
           // Temporarily disables floating-point exception inside this scope.
           ScopedDisableFPEx disableFp_;
           // ...
       }
       // Floating-point exception is enabled again
       // ...
    \endrst
*/
class ScopedDisableFPEx {
public:
    ScopedDisableFPEx() { disableFPEx(); }
    ~ScopedDisableFPEx() { enableFPEx(); }
    LM_DISABLE_COPY_AND_MOVE(ScopedDisableFPEx)
};

/*!
    \brief Scoped guard of `init` and `shutdown` functions.
*/
class ScopedInit {
public:
    ScopedInit(const Json& prop = {}) { init(prop); }
    ~ScopedInit() { shutdown(); }
    LM_DISABLE_COPY_AND_MOVE(ScopedInit)
};

/*!
    @}
*/

LM_NAMESPACE_END(exception)
LM_NAMESPACE_END(LM_NAMESPACE)

// ------------------------------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \addtogroup exception
    @{
*/

/*!
    \brief Error code.

    \rst
    Represents the types of errors used in the framework.
    This error code is associated with :class:`Exception`.
    \endrst
*/
enum class Error {
    None,               //!< Used for other errors.
    InvalidArgument,    //!< Argument is invalid.
    Unimplemented,      //!< Feature is unimplemented.
};


/*!
    \brief Exception.

    \rst
    Represents the exception used in the framework.
    The user want to use convenience macro :cpp:func:`LM_THROW`
    instead of throwing this exception directly.
    \endrst
*/
class Exception : public std::exception {
private:
    Error error_;
    std::string file_;
    int line_;
    std::string message_;

public:
    /*!
        \brief Constructor.
        \param error Error code.
        \param file File name.
        \param line Line of code.
        \param message Error message.
        \param ... Arguments to format the message.

        \rst
        Constructs the exception with error type and error message.
        \endrst
    */
    template <typename... Args>
    Exception(Error error, const std::string& file, int line, const std::string& message)
        : error_(error)
        , file_(file)
        , line_(line)
        , message_(message)
    {}

    /*!
        \brief Constructor with arguments.
        \param error Error code.
        \param file File name.
        \param line Line of code.
        \param message Error message.
        \param ... Arguments to format the message.
        
        \rst
        Constructs the exception with error type and error message.
        The constructor also takes additional arguments to format the message.
        \endrst
    */
    template <typename... Args>
    Exception(Error error, const std::string& file, int line, const std::string& message, const Args& ... args) 
        : error_(error)
        , file_(file)
        , line_(line)
        , message_(fmt::format(message, args...))
    {}

    const char* what() const noexcept {
        // Compose error message with error code
        const auto errorCodeStr = [this]() -> std::string {
            if (error_ == Error::None) {
                return "None";
            }
            if (error_ == Error::InvalidArgument) {
                return "InvalidArgumnt";
            }
            if (error_ == Error::Unimplemented) {
                return "Unimplemented";
            }
            LM_UNREACHABLE_RETURN();
        }();
        return fmt::format("{} [type='{}', file='{}', line='{}']", message_, errorCodeStr).c_str();
    }
};

/*!
    @}
*/

LM_NAMESPACE_END(LM_NAMESPACE)

// ------------------------------------------------------------------------------------------------

/*!
    \addtogroup exception
    @{
*/

/*!
    \brief Throw exception.
    \param error Error code.
    \param message Error message.
    \param ... Arguments to format the message.

    \rst
    Convenience macro to throw :class:`Exception`. 
    This macro reports the file and line of the code where the exception being raised.
    \endrst
*/
#define LM_THROW_EXCEPTION(error, message, ...) \
    throw LM_NAMESPACE::Exception(error, __FILE__, __LINE__, message, __VA_ARGS__)

/*!
    \brief Throw exception without message.
    \param error Error code.
    \param message Error message.
    \param ... Arguments to format the message.

    \rst
    Convenience macro to throw :class:`Exception`.
    This macro reports the file and line of the code where the exception being raised.
    \endrst
*/
#define LM_THROW_EXCEPTION_WITHOUT_MESSSAGE(error) \
    throw LM_NAMESPACE::Exception(error, __FILE__, __LINE__, \
        "You may find a detailed introspection of this error in the log output.")

/*!
    @}
*/