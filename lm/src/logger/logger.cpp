/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/detail/logger_context.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE::log)

using Instance = comp::detail::ContextInstance<detail::LoggerContext>;

LM_PUBLIC_API void init(const std::string& type, const Json& prop) {
    Instance::init(type, prop);
}

LM_PUBLIC_API void shutdown() {
    Instance::shutdown();
}

LM_PUBLIC_API void log(LogLevel level, const char* filename, int line, const char* message) {
    Instance::get().log(level, filename, line, message);
}

LM_PUBLIC_API void updateIndentation(int n) {
    Instance::get().updateIndentation(n);
}

LM_NAMESPACE_END(LM_NAMESPACE::log)
