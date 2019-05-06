/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

// ----------------------------------------------------------------------------

// Debug mode flag
#ifndef NDEBUG
    #define LM_DEBUG_MODE 1
#else
    #define LM_DEBUG_MODE 0
#endif

// ----------------------------------------------------------------------------

// Platform flag
#ifdef _WIN32
    #define LM_PLATFORM_WINDOWS 1
#else
    #define LM_PLATFORM_WINDOWS 0
#endif
#ifdef __linux
    #define LM_PLATFORM_LINUX 1
#else
    #define LM_PLATFORM_LINUX 0
#endif
#ifdef __APPLE__
    #define LM_PLATFORM_APPLE 1
#else
    #define LM_PLATFORM_APPLE 0
#endif

// ----------------------------------------------------------------------------

// Compiler flag
#ifdef _MSC_VER
    #define LM_COMPILER_MSVC 1
#else
    #define LM_COMPILER_MSVC 0
#endif
#ifdef __INTELLISENSE__
    #define LM_INTELLISENSE 1
#else
    #define LM_INTELLISENSE 0
#endif
#if defined(__GNUC__) || defined(__MINGW32__)
    #define LM_COMPILER_GCC 1
#else
    #define LM_COMPILER_GCC 0
#endif
#ifdef __clang__
    #define LM_COMPILER_CLANG 1
    #if LM_COMPILER_GCC
        // clang defines __GNUC__
        #undef LM_COMPILER_GCC
        #define LM_COMPILER_GCC 0
    #endif
#else
    #define LM_COMPILER_CLANG 0
#endif

// ----------------------------------------------------------------------------

// Architecture flag
#if LM_COMPILER_MSVC
    #ifdef _M_IX86
        #define LM_ARCH_X86 1
    #else
        #define LM_ARCH_X86 0
    #endif
    #ifdef _M_X64
        #define LM_ARCH_X64 1
    #else
        #define LM_ARCH_X64 0
    #endif
#elif LM_COMPILER_GCC || LM_COMPILER_CLANG
    #ifdef __i386__
        #define LM_ARCH_X86 1
    #else
        #define LM_ARCH_X86 0
    #endif
    #ifdef __x86_64__
        #define LM_ARCH_X64 1
    #else
        #define LM_ARCH_X64 0
    #endif
#endif

// ----------------------------------------------------------------------------

// Disable some warnings
#if LM_PLATFORM_WINDOWS
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #pragma warning(disable:4505)  // unreferenced local function has been removed
#endif

// ----------------------------------------------------------------------------

// Dynamic library import and export
#if LM_COMPILER_MSVC
    #ifdef LM_EXPORTS
        #define LM_PUBLIC_API __declspec(dllexport)
    #else
        #define LM_PUBLIC_API __declspec(dllimport)
    #endif
    #define LM_HIDDEN_API
#elif LM_COMPILER_GCC || LM_COMPILER_CLANG
    #ifdef LM_EXPORTS
        #define LM_PUBLIC_API __attribute__ ((visibility("default")))
        #define LM_HIDDEN_API __attribute__ ((visibility("hidden")))
    #else
        #define LM_PUBLIC_API
        #define LM_HIDDEN_API
    #endif
#else
    #define LM_PUBLIC_API
    #define LM_HIDDEN_API
#endif

// ----------------------------------------------------------------------------

#pragma region Force inline
#if LM_COMPILER_MSVC
    #define LM_INLINE __forceinline
#elif LM_COMPILER_GCC || LM_COMPILER_CLANG
    #define LM_INLINE inline __attribute__((always_inline))
#endif
#pragma endregion

// ----------------------------------------------------------------------------

// Alignment
#if LM_COMPILER_MSVC
    #define LM_ALIGN(x) __declspec(align(x))
#elif LM_COMPILER_GCC || LM_COMPILER_CLANG
    #define LM_ALIGN(x) __attribute__((aligned(x)))
#endif
#define LM_ALIGN_16 LM_ALIGN(16)
#define LM_ALIGN_32 LM_ALIGN(32)

// ----------------------------------------------------------------------------

// Calling convension
#if LM_COMPILER_MSVC
    #define LM_CDECL __cdecl
#elif LM_COMPILER_GCC || LM_COMPILER_CLANG
    #define LM_CDECL [[gnu::cdecl]]
#endif

// ----------------------------------------------------------------------------

// Helper macros
#define LM_TOKENPASTE(x, y) x ## y
#define LM_TOKENPASTE2(x, y) LM_TOKENPASTE(x, y)
#define LM_STRINGIFY(x) #x
#define LM_STRINGIFY2(x) LM_STRINGIFY(x)
#define LM_UNREACHABLE() assert(0)
#define LM_UNREACHABLE_RETURN() assert(0); return {}

#if LM_PLATFORM_WINDOWS
    #define LM_PRAGMA(x) __pragma(x)
#else
    #define LM_PRAGMA(x) _Pragma(LM_STRINGIFY(x))
#endif

#define LM_SAFE_DELETE(val) if ((val) != nullptr) { delete (val); (val) = nullptr; }
#define LM_SAFE_DELETE_ARRAY(val) if ((val) != nullptr) { delete[] (val); (val) = nullptr; }

#define LM_DISABLE_COPY_AND_MOVE(TypeName) \
    TypeName(const TypeName &) = delete; \
    TypeName(TypeName&&) = delete; \
    auto operator=(const TypeName&) -> void = delete; \
    auto operator=(TypeName&&) -> void = delete;

#define LM_DISABLE_CONSTRUCT(TypeName) \
    TypeName() = delete; \
    LM_DISABLE_COPY_AND_MOVE(TypeName);

#define LM_TBA() LM_PRAGMA(error ("TBA"))
#define LM_TBA_RUNTIME() throw std::runtime_error("TBA")

// ----------------------------------------------------------------------------

// Framework namespace
#ifndef LM_NAMESPACE_BEGIN
    #define LM_NAMESPACE_BEGIN(name) namespace name {
#endif
#ifndef LM_NAMESPACE_END
    #define LM_NAMESPACE_END(name) }
#endif
#ifndef LM_FORWARD_DECLARE_WITH_NAMESPACE
    #define LM_FORWARD_DECLARE_WITH_NAMESPACE(name, declaration) \
        LM_NAMESPACE_BEGIN(name) \
        declaration; \
        LM_NAMESPACE_END(name)
#endif
#ifndef LM_NAMESPACE
#define LM_NAMESPACE lm
#endif

// ----------------------------------------------------------------------------

// Unused macro
// Only use this macro to specify variables being used.
// Using this macro for expressions might lead unexpected behavior.
// https://stackoverflow.com/questions/1486904/how-do-i-best-silence-a-warning-about-unused-variables/44106902
#define LM_UNUSED(...) { (decltype(LM_NAMESPACE::detail::unused(__VA_ARGS__)))0; }
LM_NAMESPACE_BEGIN(LM_NAMESPACE)
LM_NAMESPACE_BEGIN(detail)
template <typename... Args>
inline void unused(Args&&...) {}
LM_NAMESPACE_END(detail)
LM_NAMESPACE_END(LM_NAMESPACE)

// ----------------------------------------------------------------------------

#pragma warning(push)
#pragma warning(disable:4127)  // conditional expression is constant
#include "ext/nlohmann/json.hpp"
#pragma warning(pop)

// cereal library
#include "ext/cereal/cereal.hpp"
#define LM_USE_JSON_ARCHIVE 0
#if LM_USE_JSON_ARCHIVE
#include "ext/cereal/archives/json.hpp"
#else
#include "ext/cereal/archives/portable_binary.hpp"
#endif

// fmt library
#define FMT_HEADER_ONLY
#include "ext/fmt/format.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// ----------------------------------------------------------------------------

// Forward declarations
struct ComponentDeleter;  // component.h
class Component;
class Assets;             // assets.h
class Mesh;               // mesh.h
class Material;           // material.h
class Light;              // light.h
class Camera;             // camera.h
struct FilmBuffer;        // film.h
class Film;
struct PointGeometry;     // surface.h
struct SurfacePoint;      // scene.h
struct RaySample;
struct LightSample;
class Scene;
class Renderer;           // renderer.h
LM_FORWARD_DECLARE_WITH_NAMESPACE(comp::detail, struct Access)

// ----------------------------------------------------------------------------

// Default floating point type
#define LM_SINGLE_PRECISION 0
#define LM_DOUBLE_PRECISION 1
#if LM_SINGLE_PRECISION
using Float = float;
#elif LM_DOUBLE_PRECISION
using Float = double;
#else
#error "Invalid precision mode"
#endif

// ----------------------------------------------------------------------------

// JSON type
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

// ----------------------------------------------------------------------------

// Default input/output archives deligated to cereal library.
#if LM_USE_JSON_ARCHIVE
using InputArchive = cereal::JSONInputArchive;
using OutputArchive = cereal::JSONOutputArchive;
#else
using InputArchive = cereal::PortableBinaryInputArchive;
using OutputArchive = cereal::PortableBinaryOutputArchive;
#endif

// ----------------------------------------------------------------------------

LM_NAMESPACE_END(LM_NAMESPACE)
