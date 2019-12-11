/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "component.h"
#include "math.h"
#include <regex>
#include <fstream>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \addtogroup user
    @{
*/

/*!
    \brief Initialize the framework.
    \param prop Properties for configuration.
    \see `example/blank.cpp`

    \rst
    The framework must be initialized with this function before any use of other APIs.
    The properties are passed as JSON format and used to initialize
    the internal subsystems of the framework.
    This function initializes some subsystems with default types.
    If you want to configure the subsystem, you want to call each ``init()`` function afterwards.
    \endrst
*/
LM_PUBLIC_API void init(const Json& prop = {});

/*!
    \brief Shutdown the framework.

    \rst
    This function explicit shutdowns the framework.
    \endrst
*/
LM_PUBLIC_API void shutdown();

/*!
    \brief Reset the internal state of the framework.

    \rst
    This function resets underlying states including assets and scene to the initial state.
    The global contexts remains same.
    \endrst
*/
LM_PUBLIC_API void reset();

/*!
    \brief Print information about Lightmetrica.
*/
LM_PUBLIC_API void info();

/*!
    \brief Get underlying collection of assets.
    \return Instance.
*/
LM_PUBLIC_API AssetGroup* assets();


/*!
    @}
*/

LM_NAMESPACE_END(LM_NAMESPACE)
