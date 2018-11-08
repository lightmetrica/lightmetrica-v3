/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include <pch.h>
#include <lm/exception.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE::exception)

using Instance = comp::detail::ContextInstance<detail::ExceptionContext>;

LM_PUBLIC_API void init(const std::string& type, const Json& prop) {
    Instance::init(type, prop);
}

LM_PUBLIC_API void shutdown() {
    Instance::shutdown();
}

LM_PUBLIC_API void enableFPEx() {
    Instance::get().enableFPEx();
}

LM_PUBLIC_API void disableFPEx() {
    Instance::get().disableFPEx();
}

LM_PUBLIC_API void stackTrace() {
    Instance::get().stackTrace();
}

LM_NAMESPACE_END(LM_NAMESPACE::exception)
