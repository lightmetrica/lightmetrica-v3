/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include "common.h"
#include <fmt/format.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)
LM_NAMESPACE_BEGIN(version)

/*!
    \addtogroup version
    @{
*/

/*!
    \brief Major version.
*/
LM_PUBLIC_API int majorVersion();

/*!
    \brief Minor version.
*/
LM_PUBLIC_API int minorVersion();

/*!
    \brief Patch version.
*/
LM_PUBLIC_API int patchVersion();

/*!
    \brief Revision string.
*/
LM_PUBLIC_API std::string revision();

/*!
    \brief Build timestamp.
*/
LM_PUBLIC_API std::string buildTimestamp();

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
static std::string formatted() {
    return fmt::format("{}.{}.{} (rev. {})",
        majorVersion(), minorVersion(), patchVersion(), revision());
}

/*!
    @}
*/

LM_NAMESPACE_END(version)
LM_NAMESPACE_END(LM_NAMESPACE)
