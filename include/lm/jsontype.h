/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "common.h"

#pragma warning(push)
#pragma warning(disable:4127)  // conditional expression is constant
#include <nlohmann/json.hpp>
#pragma warning(pop)

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \addtogroup json
    @{
*/

/*!
    \brief JSON type.

    \rst
    We use nlohmann_json library to represent various properties.
    This type is mainly used to communicate data between user and framework through API.
    \endrst
*/
using Json = nlohmann::basic_json<
    std::map,           // Object type
    std::vector,        // Arrray type
    std::string,        // String type
    bool,               // Boolean type
    std::int64_t,       // Signed integer type
    std::uint64_t,      // Unsigned integer type
    Float,              // Floating point type
    std::allocator,
    nlohmann::adl_serializer>;

/*!
    @}
*/

LM_NAMESPACE_END(LM_NAMESPACE)