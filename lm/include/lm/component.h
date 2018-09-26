/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "common.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// ----------------------------------------------------------------------------

/*!
    Base component.
    Base class of extendable components.
*/
class Component {};

// ----------------------------------------------------------------------------

// Component API
namespace component {

///! Create component with specific interface type.
template <typename InterfaceT>
InterfaceT* create(const char* key) {
    
}

///! Create a new component.
LM_PUBLIC_API Component* create(const char* key);

// ----------------------------------------------------------------------------

namespace detail {

using ComponentCreateFunctionType  = std::function<Component*()>;
using ComponentReleaseFunctionType = std::function<void(Component*)>;

///! Register a component
LM_PUBLIC_API void reg(
    const char* key,
    const ComponentCreateFunctionType& createFunc,
    const ComponentReleaseFunctionType& releaseFunc);

///! Unregister a component
LM_PUBLIC_API void unreg(const char* key);

///! Get release function
LM_PUBLIC_API ComponentReleaseFunctionType releaseFunc(const char* key);

///! Load plugins inside a given directory
LM_PUBLIC_API void loadPlugins(const char* directory);

///! Unload loaded plugins
LM_PUBLIC_API void unloadPlugins();

}
}

// ----------------------------------------------------------------------------

LM_NAMESPACE_END(LM_NAMESPACE)
