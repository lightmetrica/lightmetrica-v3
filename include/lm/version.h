/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "common.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)
LM_NAMESPACE_BEGIN(version)

/*!
    \addtogroup version
    @{
*/

//! Major version.
LM_PUBLIC_API int major_version();

//! Minor version.
LM_PUBLIC_API int minor_version();

//! Patch version.
LM_PUBLIC_API int patch_version();

//! Revision string.
LM_PUBLIC_API std::string revision();

//! Build timestamp.
LM_PUBLIC_API std::string build_timestamp();

//! Platform.
LM_PUBLIC_API std::string platform();

//! Architecture.
LM_PUBLIC_API std::string architecture();

//! Formatted version string.
LM_PUBLIC_API std::string formatted();

/*!
    @}
*/

LM_NAMESPACE_END(version)
LM_NAMESPACE_END(LM_NAMESPACE)
