/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/version.h>
#include <versiondef.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE::version)

LM_PUBLIC_API int majorVersion() {
    return LM_VERSION_MAJOR;
}

LM_PUBLIC_API int minorVersion() {
    return LM_VERSION_MINOR;
}

LM_PUBLIC_API int patchVersion() {
    return LM_VERSION_PATCH;
}

LM_PUBLIC_API std::string revision() {
    return LM_VERSION_REVISION;
}

LM_PUBLIC_API std::string buildTimestamp() {
    return LM_BUILD_TIMESTAMP;
}

LM_PUBLIC_API std::string platform() {
    #if LM_PLATFORM_WINDOWS
    return "Windows";
    #elif LM_PLATFORM_LINUX
    return "Linux";
    #elif LM_PLATFORM_APPLE
    return "Apple";
    #endif
}

LM_PUBLIC_API std::string architecture() {
    #if LM_ARCH_X64
    return "x64";
    #elif LM_ARCH_X86
    return "x86";
    #endif
}

LM_PUBLIC_API std::string formatted() {
    return fmt::format("{}.{}.{} (rev. {})",
        majorVersion(), minorVersion(), patchVersion(), revision());
}

LM_NAMESPACE_END(LM_NAMESPACE::version)
