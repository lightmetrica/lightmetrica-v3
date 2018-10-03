/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "common.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// ----------------------------------------------------------------------------

/*!
    \brief Base component.

    Base class of all components in Lightmetrica.
    All components interfaces and implementations must inherit this class.
*/
class Component {
public:
    //! Factory function type
    using CreateFunction  = std::function<Component*()>;
    //! Release function type
    using ReleaseFunction = std::function<void(Component*)>;

public:
    //! Name of the component instance.
    std::string key_;
    //! Factory function of the component instance.
    CreateFunction  createFunc_ = nullptr;
    //! Release function of the component instance.
    ReleaseFunction releaseFunc_ = nullptr;
    //! Underlying reference to owner object (if any)
    std::any ownerRef_;

public:
    Component() = default;
    virtual ~Component() = default;
    LM_DISABLE_COPY_AND_MOVE(Component);

public:
    /*!
        \brief 
    */
    virtual void construct() = 0;
    
    /*!
        \brief 
    */
    virtual void save() = 0;

};

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(comp)

/*!
    \brief Unique pointer for component instances.
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
    auto* inst = detail::createComp(key);
    if (!inst) { return UniquePtr<InterfaceT>(nullptr, nullptr); }
    return UniquePtr<InterfaceT>(dynamic_cast<InterfaceT*>(inst), inst->releaseFunc_);
}

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(detail)

/*!
    \brief Create a component and its deleter.
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
    \brief Scope guard of `loadPlugins` and `loadPlugins`.
*/
class ScopedLoadPlugin {
private:
    bool valid_ = true;

public:
    ScopedLoadPlugin(const char* path) : ScopedLoadPlugin({ path }) {}
    ScopedLoadPlugin(std::initializer_list<const char*> paths) {
        for (auto path : paths) {
            valid_ |= loadPlugin(path);
            if (!valid_) { break; }
        }
    }
    ~ScopedLoadPlugin() { unloadPlugins(); }
    LM_DISABLE_COPY_AND_MOVE(ScopedLoadPlugin);

public:
    bool valid() const { return valid_; }
};

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
        // Register factory function
        reg(key,
            []() -> Component* { return new ImplType; },
            [](Component* p) { LM_SAFE_DELETE(p); });
    }
    ~ImplEntry_() { unreg(key_.c_str()); }
};

LM_NAMESPACE_END(detail)
LM_NAMESPACE_END(comp)
LM_NAMESPACE_END(LM_NAMESPACE)

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
            static const LM_NAMESPACE::comp::detail::ImplEntry_<ImplType>& reg_; \
        }; \
        const LM_NAMESPACE::comp::detail::ImplEntry_<ImplType>& \
            ImplEntry_Init_<ImplType>::reg_ = LM_NAMESPACE::comp::detail::ImplEntry_<ImplType>::instance(Key); \
    }
