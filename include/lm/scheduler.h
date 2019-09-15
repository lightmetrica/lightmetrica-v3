/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "component.h"
#include <chrono>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)
LM_NAMESPACE_BEGIN(scheduler)

/*!
    \addtogroup scheduler
    @{
*/

/*!
    \brief Sample-per-pixel scheduler.

    \rst
    A scheduler for the rendering techniues based on sample-per-pixel (SPP) loop.
    This scheduler guarantees the equal number of samples are processed for each pixel.
    \endrst
*/
class SPPScheduler : public Component {
public:
    /*!
        \brief Callback function for parallel loop.
        \param pixelIndex Pixel index.
        \param sampleIndex Pixel sample index.
        \param threadid Thread index.
    */
    using ProcessFunc = std::function<void(long long pixelIndex, long long sampleIndex, int threadid)>;

    /*!
        \brief Dispatch scheduler.
        \param pixels Number of pixels.
        \param process Callback function for parallel loop.
        \return Processed samples per pixel.
    */
    virtual long long run(long long numPixels, const ProcessFunc& process) const = 0;
};

/*!
    \brief Sample-per-image scheduler.

    \rst
    A scheduler for the rendering techniues based on sample-per-image (SPI) loop.
    \endrst
*/
class SPIScheduler : public Component {
public:
    /*!
        \brief Callback function for parallel loop.
        \param sampleIndex Sample index.
        \param threadid Thread index.
    */
    using ProcessFunc = std::function<void(long long sampleIndex, int threadid)>;
    
    /*!
        \brief Dispatch scheduler.
        \param process Callback function for parallel loop.
        \return Processed samples.
    */
    virtual long long run(const ProcessFunc& process) const = 0;
};

/*!
    @}
*/

LM_NAMESPACE_END(scheduler)
LM_NAMESPACE_END(LM_NAMESPACE)
