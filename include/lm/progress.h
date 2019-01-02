/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "component.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)
LM_NAMESPACE_BEGIN(progress)

/*!
    \addtogroup progress
    @{
*/

//! Default progress reporter type
constexpr const char* DefaultType = "progress::default";

/*!
    \brief Initialize progress reporter context.
*/
LM_PUBLIC_API void init(const std::string& type = DefaultType, const Json& prop = {});

/*!
    \brief Shutdown progress reporter context.
*/
LM_PUBLIC_API void shutdown();

/*!
    \brief Start progress reporting.
*/
LM_PUBLIC_API void start(long long total);

/*!
    \brief End progress reporting.
*/
LM_PUBLIC_API void end();

/*!
    \brief Update progress with specific values in [0,1].
*/
LM_PUBLIC_API void update(long long processed);

/*!
    Scoped guard of `start` and `end` functions.
*/
class ScopedReport {
public:
    ScopedReport(long long total) { start(total); }
    ~ScopedReport() { end(); }
    LM_DISABLE_COPY_AND_MOVE(ScopedReport)
};

/*!
    @}
*/

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(detail)

/*!
    \addtogroup progress
    @{
*/

/*!
    \brief Progress context.
*/
class ProgressContext : public Component {
public:
    virtual void start(long long total) = 0;
    virtual void update(long long processed) = 0;
    virtual void end() = 0;
};

/*!
    @}
*/

LM_NAMESPACE_END(detail)

// ----------------------------------------------------------------------------

LM_NAMESPACE_END(progress)
LM_NAMESPACE_END(LM_NAMESPACE)
