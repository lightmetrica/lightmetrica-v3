/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "common.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(component)
class Impl_;
LM_NAMESPACE_END(component)

// ----------------------------------------------------------------------------

/*!
    Base component.
    Base class of extendable components.
*/
class Component {
private:

    friend class component::Impl_;

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

LM_NAMESPACE_BEGIN(component)

///! Create component with specific interface type.
template <typename InterfaceT>
InterfaceT* create(const char* key) {
    
}

///! Create a new component.
LM_PUBLIC_API Component* create(const char* key);

LM_NAMESPACE_END(component)

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(component::detail)

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

LM_NAMESPACE_END(component::detail)

// ----------------------------------------------------------------------------

LM_NAMESPACE_END(LM_NAMESPACE)
