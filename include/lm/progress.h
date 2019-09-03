/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
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
    \param type Type of progress reporter subsystem.
    \param prop Properties for configuration.

    \rst
    This function initializes exception subsystem of the framework.
    The function is implicitly called by the framework
    so the user do not want to explicitly call this function.
    \endrst
*/
LM_PUBLIC_API void init(const std::string& type = DefaultType, const Json& prop = {});

/*!
    \brief Shutdown progress reporter context.

    \rst
    This function shutdowns the exception subsystem.
    You do not want to call this function because it is called implicitly by the framework. 
    \endrst
*/
LM_PUBLIC_API void shutdown();

/*!
	\brief Progress reporting mode.
*/
enum class ProgressMode {
	Samples,
	Time
};

/*!
    \brief Start progress reporting.
    \param total Total number of iterations.

    \rst
    This function specifies the start of the progress reporting.
    The argument ``total`` is necessary to calculate the ratio of progress
    over the entire workload.
    You may use :class:`ScopedReport` class to automatically enable/disable
    the floating point exception inside a scope.
    \endrst
*/
LM_PUBLIC_API void start(ProgressMode mode, long long total, double totalTime);

/*!
    \brief End progress reporting.

    \rst
    This function specifies the end of the progress reporting.
    You may use :class:`ScopedReport` class to automatically enable/disable
    the floating point exception inside a scope.
    \endrst
*/
LM_PUBLIC_API void end();

/*!
    \brief Update progress.
    \param processed Processed iterations.

    \rst
    This function notifies the update of the progress to the subsystem.
    ``processed`` must be between 0 to ``total`` specified
    in the :func:`lm::progress::start` function.
    \endrst
*/
LM_PUBLIC_API void update(long long processed);

/*!
    \brief Update time progress.
*/
LM_PUBLIC_API void updateTime(Float elapsed);

/*!
    \brief Scoped guard of `start` and `end` functions.
*/
class ScopedReport {
public:
    ScopedReport(long long total) { start(ProgressMode::Samples, total, -1); }
    ~ScopedReport() { end(); }
    LM_DISABLE_COPY_AND_MOVE(ScopedReport)
};

/*!
	\brief Scoped guard of `startTime` and `end` functions.
*/
class ScopedTimeReport {
public:
	ScopedTimeReport(double totalTime) { start(ProgressMode::Time, -1, totalTime); }
	~ScopedTimeReport() { end(); }
	LM_DISABLE_COPY_AND_MOVE(ScopedTimeReport)
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
    
    \rst
    You may implement this interface to implement user-specific progress reporting subsystem.
    Each virtual function corresponds to API call with a free function
    inside ``progress`` namespace.
    \endrst
*/
class ProgressContext : public Component {
public:
    virtual void start(ProgressMode mode, long long total, double totalTime) = 0;
    virtual void update(long long processed) = 0;
    virtual void updateTime(Float elapsed) = 0;
    virtual void end() = 0;
};

/*!
    @}
*/

LM_NAMESPACE_END(detail)

// ----------------------------------------------------------------------------

LM_NAMESPACE_END(progress)
LM_NAMESPACE_END(LM_NAMESPACE)
