/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "common.h"
#include <any>
#include <memory>
#include <string>
#include <nlohmann/json.hpp>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// ----------------------------------------------------------------------------

// JSON type
using json = nlohmann::json;

// ----------------------------------------------------------------------------

LM_FORWARD_DECLARE_WITH_NAMESPACE(comp::detail, class Impl);
LM_FORWARD_DECLARE_WITH_NAMESPACE(py::detail, class Impl);

// ----------------------------------------------------------------------------

/*!
    \brief Base component.

    Base class of all components in Lightmetrica.
    All components interfaces and implementations must inherit this class.
*/
class Component {
private:
    friend class comp::detail::Impl;
    friend class py::detail::Impl;

public:
    //! Factory function type
    using CreateFunction  = std::function<Component*()>;

    //! Release function type
    using ReleaseFunction = std::function<void(Component*)>;

    //! Unique pointer for component instances.
    template <typename InterfaceT>
    using UniquePtr = std::unique_ptr<InterfaceT, ReleaseFunction>;

private:
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
        \brief Make UniquePtr from an component instance.
    */
    template <typename InterfaceT>
    static UniquePtr<InterfaceT> makeUnique(Component* inst) {
        return UniquePtr<InterfaceT>(dynamic_cast<InterfaceT*>(inst), inst->releaseFunc_);
    }

    /*!
        \brief Cast to specific component interface type.
    */
    template <typename InterfaceT>
    const InterfaceT* cast() const { return dynamic_cast<InterfaceT*>(this); }
    
    /*!
        \brief Cast to specific component interface type.
    */
    template <typename InterfaceT>
    InterfaceT* cast() { return dynamic_cast<InterfaceT*>(this); }

public:
    /*!
        \brief Construct a component.
    */
    virtual bool construct(const json& prop, Component* parent = nullptr) { return true; }
    
    /*!
        \brief Deserialize a component.
    */
    virtual void load(std::istream& stream, Component* parent = nullptr) {}

    /*!
        \brief Serialize a component.
    */
    virtual void save(std::ostream& stream) const {}

    /*!
        \brief Get underlying component instance.
    */
    virtual Component* underlying(const char* name = nullptr) const { return nullptr; }

    /*!
        \brief Get underlying component instance with specific interface type.
    */
    template <typename UnderlyingInterfaceT>
    UnderlyingInterfaceT* underlying(const char* name) const {
        return dynamic_cast<UnderlyingInterfaceT*>(underlying(name));
    }
};

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(comp)

/*!
    \brief Create component with specific interface type.
    \tparam InterfaceT Component interface type.
    \param key Name of the implementation.
*/
template <typename InterfaceT>
Component::UniquePtr<InterfaceT> create(const char* key) {
    auto* inst = detail::createComp(key);
    if (!inst) {
        return Component::UniquePtr<InterfaceT>(nullptr, nullptr);
    }
    return Component::makeUnique<InterfaceT>(inst);
}

/*!
    \brief Create component with construction with given properties.
    \tparam InterfaceT Component interface type.
    \param key Name of the implementation.
    \param prop Properties.
    \param parent Parent component instance.
*/
template <typename InterfaceT>
Component::UniquePtr<InterfaceT> create(const char* key, const json& prop, Component* parent = nullptr) {
    auto inst = create<InterfaceT>(key);
    if (!inst || !inst->construct(prop, parent)) {
        return Component::UniquePtr<InterfaceT>(nullptr, nullptr);
    }
    return inst;
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
