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

/*!
    \brief Major version.
*/
LM_PUBLIC_API int major_version();

/*!
    \brief Minor version.
*/
LM_PUBLIC_API int minor_version();

/*!
    \brief Patch version.
*/
LM_PUBLIC_API int patch_version();

/*!
    \brief Revision string.
*/
LM_PUBLIC_API std::string revision();

/*!
    \brief Build timestamp.
*/
LM_PUBLIC_API std::string build_timestamp();

/*!
    \brief Platform.
*/
LM_PUBLIC_API std::string platform();

/*!
    \brief Architecture.
*/
LM_PUBLIC_API std::string architecture();

/*!
    \brief Formatted version string.
*/
LM_PUBLIC_API std::string formatted();

/*!
    @}
*/

LM_NAMESPACE_END(version)
LM_NAMESPACE_END(LM_NAMESPACE)
