/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "common.h"

// ------------------------------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(cereal)
// Forward declaration of archive types
class PortableBinaryOutputArchive;
class PortableBinaryInputArchive;
LM_NAMESPACE_END(cereal)

// ------------------------------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \addtogroup serial
    @{
*/

/*!
    \brief Default input archive used by lm::Component.

    \rst
    We use cereal's portal binary archive as a default input archive type.
    This type is used as an argument type of :cpp:func:`lm::Component::load` function.
    \endrst
*/
using InputArchive = cereal::PortableBinaryInputArchive;

/*!
    \brief Default output archive used by lm::Component.

    \rst
    We use cereal's portal binary archive as a default output archive type.
    This type is used as an argument type of :cpp:func:`lm::Component::save` function.
    \endrst
*/
using OutputArchive = cereal::PortableBinaryOutputArchive;

/*!
    @}
*/

LM_NAMESPACE_END(LM_NAMESPACE)