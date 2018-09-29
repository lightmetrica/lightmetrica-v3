/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "common.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(comp)
class Impl_;
LM_NAMESPACE_END(comp)

// ----------------------------------------------------------------------------

/*!
    \brief Base component.

    Base class of all components in Lightmetrica.
    All components interfaces and implementations must inherit this class.
*/
class Component {
private:
    friend class comp::Impl_;

public:
    // Create and release function types
    using CreateFunction = Component* (*)();
    using ReleaseFunction = void(*)(Component*);

private:
    // Create and release functions
    std::string key_;
    CreateFunction  createFunc_ = nullptr;
    ReleaseFunction releaseFunc_ = nullptr;

public:
    Component() = default;
    virtual ~Component() = default;
    LM_DISABLE_COPY_AND_MOVE(Component);
};

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(comp)

/*!
    \brief Unique pointer type of component interfaces.
*/
template <typename InterfaceT>
using UniquePtr = std::unique_ptr<InterfaceT, Component::ReleaseFunction>;

/*!
    \brief Create component with specific interface type.
    \tparam InterfaceT Component interface type.
    \param key Name of the implementation.
*/
template <typename InterfaceT>
UniquePtr<InterfaceT> create(const char* key) {
    auto* p = static_cast<InterfaceT*>(detail::createComp(key));
    if (!p) { return UniquePtr<InterfaceT>(nullptr, nullptr); }
    return UniquePtr<InterfaceT>(p, detail::releaseFunc(key));
}

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(detail)

/*!
    \brief Create a new component.
    \param key Name of the implementation.
*/
LM_PUBLIC_API Component* createComp(const char* key);

/*!
    \brief Register a component.
*/
LM_PUBLIC_API void reg(
    const char* key,
    const Component::CreateFunction& createFunc,
    const Component::ReleaseFunction& releaseFunc);

/*!
    \brief Unregister a component.
*/
LM_PUBLIC_API void unreg(const char* key);

/*!
    \brief Get release function.
*/
LM_PUBLIC_API Component::ReleaseFunction releaseFunc(const char* key);

/*!
    \brief Load a plugin.
*/
LM_PUBLIC_API bool loadPlugin(const char* path);

/*!
    \brief Load plugins inside a given directory.
*/
LM_PUBLIC_API void loadPlugins(const char* directory);

/*!
    \brief Unload loaded plugins.
*/
LM_PUBLIC_API void unloadPlugins();

// ----------------------------------------------------------------------------

/*!
    \brief Helps registration of an implementation.
*/
template <typename ImplType>
class ImplEntry_ {
private:
    std::string key_;
public:
    static ImplEntry_<ImplType>& instance(const char* key) {
        static ImplEntry_<ImplType> instance(key);
        return instance;
    }
private:
    ImplEntry_(const char* key) : key_(key) {
        reg(key,
            []() -> Component* { return new ImplType; },
            [](Component* p) -> void {
                auto* p2 = static_cast<ImplType*>(p);
                LM_SAFE_DELETE(p2); p = nullptr;
            });
    }
    ~ImplEntry_() { unreg(key_.c_str()); }
};

LM_NAMESPACE_END(detail)
LM_NAMESPACE_END(comp)

// ----------------------------------------------------------------------------

/*!
    \brief Register implementation.
    \param ImplType Component implemenation type.
    \param Key Name of the implementation.
*/
#define LM_COMP_REG_IMPL(ImplType, Key) \
	namespace { \
		template <typename T> class ImplEntry_Init_; \
		template <> class ImplEntry_Init_<ImplType> { \
            static const LM_NAMESPACE::comp::detail::ImplEntry_<ImplType>& reg_; }; \
        const LM_NAMESPACE::comp::detail::ImplEntry_<ImplType>& ImplEntry_Init_<ImplType>::reg_ = \
            LM_NAMESPACE::comp::detail::ImplEntry_<ImplType>::instance(Key); \
    }

// ----------------------------------------------------------------------------

LM_NAMESPACE_END(LM_NAMESPACE)
