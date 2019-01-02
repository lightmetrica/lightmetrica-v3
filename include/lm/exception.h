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
*/
LM_PUBLIC_API void init(const std::string& type = DefaultType, const Json& prop = {});

/*!
    \brief Shutdown exception context.
*/
LM_PUBLIC_API void shutdown();

/*!
    \brief Enable floating point exceptions.
*/
LM_PUBLIC_API void enableFPEx();

/*!
    \brief Disable floating point exceptions.
*/
LM_PUBLIC_API void disableFPEx();

/*!
    \brief Print stack trace.
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
    \brief Scoped enable/disable of floating point exception.
    Useful helper when you want to temporarily disable floating point exceptions.
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
