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
constexpr const char* DefaultType = "default";

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
    Samples,    //!< Update sample count.
    Time        //!< Update time.
};

/*!
    \brief Start progress reporting.
    \param mode Progress reporting mode.
    \param total Total number of iterations (used in Samples mode).
    \param total_time Total time (used in Time mode).

    \rst
    This function specifies the start of the progress reporting.
    The argument ``total`` is necessary to calculate the ratio of progress
    over the entire workload.
    You may use :class:`ScopedReport` class to automatically enable/disable
    the floating point exception inside a scope.
    \endrst
*/
LM_PUBLIC_API void start(ProgressMode mode, long long total, double total_time);

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
    \param elapsed Elapsed time from the start.
*/
LM_PUBLIC_API void update_time(Float elapsed);

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

LM_NAMESPACE_END(progress)
LM_NAMESPACE_END(LM_NAMESPACE)
