/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "component.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)
LM_NAMESPACE_BEGIN(exception)

// ----------------------------------------------------------------------------

/*!
    \addtogroup exception
    @{
*/

//! Default exception type
constexpr const char* DefaultType = "exception::default";

/*!
    \brief Initialize exception context.
    \param type Type of exception subsystem.
    \param prop Properties for configuration.

    \rst
    This function initializes exception subsystem of the framework.
    The function is implicitly called by the framework
    so the user do not want to explicitly call this function.
    \endrst
*/
LM_PUBLIC_API void init(const std::string& type = DefaultType, const Json& prop = {});

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
    \brief Scoped guard of `init` and `shutdown` functions.
*/
class ScopedInit {
public:
    ScopedInit(const std::string& type = DefaultType, const Json& prop = {}) { init(type, prop); }
    ~ScopedInit() { shutdown(); }
    LM_DISABLE_COPY_AND_MOVE(ScopedInit)
};

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
    @}
*/

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(detail)

/*!
    \addtogroup exception
    @{
*/

/*!
    \brief Exception context.
    
    \rst
    You may implement this interface to implement user-specific exception subsystem.
    Each virtual function corresponds to API call with a free function
    inside ``exception`` namespace.
    \endrst
*/
class ExceptionContext : public Component {
public:
    virtual void enableFPEx() = 0;
    virtual void disableFPEx() = 0;
    virtual void stackTrace() = 0;
};

/*!
    @}
*/

LM_NAMESPACE_END(detail)

// ----------------------------------------------------------------------------

LM_NAMESPACE_END(exception)
LM_NAMESPACE_END(LM_NAMESPACE)
