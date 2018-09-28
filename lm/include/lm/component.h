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
    Base component.
    Base class of extendable components.
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

///! Create component with specific interface type.
template <typename InterfaceT>
std::unique_ptr<InterfaceT, Component::ReleaseFunction> create(const char* key) {
    auto* p = static_cast<InterfaceT*>(create(key));
    if (!p) { return nullptr; }
    using ReturnT = std::unique_ptr<InterfaceT, Component::ReleaseFunction>;
    return ReturnT(p, detail::releaseFunc(key));
}

///! Create a new component.
LM_PUBLIC_API Component* create(const char* key);

LM_NAMESPACE_END(comp)

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(comp::detail)

///! Register a component
LM_PUBLIC_API void reg(
    const char* key,
    const Component::CreateFunction& createFunc,
    const Component::ReleaseFunction& releaseFunc);

///! Unregister a component
LM_PUBLIC_API void unreg(const char* key);

///! Get release function
LM_PUBLIC_API Component::ReleaseFunction releaseFunc(const char* key);

///! Load plugins inside a given directory
LM_PUBLIC_API void loadPlugins(const char* directory);

///! Unload loaded plugins
LM_PUBLIC_API void unloadPlugins();

// ----------------------------------------------------------------------------

///! Helps registration of an implementation
template <typename ImplType>
class ImplEntry_ {
private:
    std::string key_;
public:
    static ImplEntry_<ImplType>& instance(const std::string& key) {
        static ImplEntry<ImplType> instance(key);
        return instance;
    }
private:
    ImplEntry_(const char* key) : key_(key) {
        reg(key, []() -> Component* { return new ImplType; },
            [](Component* p) -> void {
                auto* p2 = static_cast<ImplType*>(p);
                LM_SAFE_DELETE(p2); p = nullptr;
            });
    }
    ~ImplEntry_() { unreg(key_.c_str()); }
};

LM_NAMESPACE_END(comp::detail)

// ----------------------------------------------------------------------------

///! Register implementation
#define LM_COMPONENT_REGISTER_IMPL(ImplType, Key) \
	namespace { \
		template <typename T> class ImplEntry_Init_; \
		template <> class ImplEntry_Init_<ImplType> { static const ImplEntry_<ImplType>& reg_; }; \
        const LM_NAMESPACE::comp::detail::ImplEntry_<ImplType>& ImplEntry_Init_<ImplType>::reg_ = \
              LM_NAMESPACE::comp::detail::ImplEntry_<ImplType>::instance(Key); \
    }

// ----------------------------------------------------------------------------

LM_NAMESPACE_END(LM_NAMESPACE)
