/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "forward.h"
#include <any>
#include <memory>
#include <string>
#include <optional>
#include <functional>
#include <nlohmann/json.hpp>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

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
    using Ptr = std::unique_ptr<InterfaceT, ReleaseFunction>;

private:
    //! Name of the component instance.
    std::string key_;

    //! Factory function of the component instance.
    CreateFunction  createFunc_ = nullptr;

    //! Release function of the component instance.
    ReleaseFunction releaseFunc_ = nullptr;

    //! Underlying reference to owner object (if any)
    std::any ownerRef_;

    //! Shortcut for the root component (if any).
    Component* context_ = nullptr;

    //! Parent component (if any)
    Component* parent_ = nullptr;

public:
    Component() = default;
    virtual ~Component() = default;
    LM_DISABLE_COPY_AND_MOVE(Component)

public:
    /*!
        \brief Context component.
    */
    Component* context() const { return context_; }

    /*!
        \brief Parent component.
    */
    Component* parent() const { return parent_; }

    /*!
        \brief Set parent.
    */
    void setParent(Component* p) {
        if (!p) { return; }
        parent_ = p;
        context_ = p->context_;
    }

public:
    /*!
        \brief Make UniquePtr from an component instance.
    */
    template <typename InterfaceT>
    static Ptr<InterfaceT> makeUnique(Component* inst) {
        return Ptr<InterfaceT>(dynamic_cast<InterfaceT*>(inst), inst->releaseFunc_);
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
    virtual bool construct(const Json& prop) { return true; }
    
    /*!
        \brief Deserialize a component.
    */
    virtual void load(std::istream& stream) {}

    /*!
        \brief Serialize a component.
    */
    virtual void save(std::ostream& stream) const {}
    
public:
    /*!
        \brief Get underlying component instance.
    */
    virtual Component* underlying(const std::string& name = "") const { return nullptr; }

    /*!
        \brief Get underlying component instance with specific interface type.
    */
    template <typename UnderlyingComponentT>
    UnderlyingComponentT* underlying(const std::string& name = "") const {
        return underlying(name)->cast<UnderlyingComponentT>();
    }

    /*!
        \brief Get underlying component instance by index.
    */
    virtual Component* underlyingAt(int index) const { return nullptr; }

    /*!
        \brief Get underlying component instance by index with specifc interface type.
    */
    template <typename UnderlyingComponentT>
    UnderlyingComponentT* underlyingAt(int index) const {
        return underlyingAt(index)->cast<UnderlyingComponentT>();
    }
    
    /*!
        \brief Process given function for each underlying component call.
    */
    virtual void foreachUnderlying(
        const std::function<void(Component* p)>& processComponent) const {}

    /*!
        \brief Process given function for each underlying component call with specific interface type. 
    */
    template <typename UnderlyingComponentT>
    void foreachUnderlying(
        const std::function<void(UnderlyingComponentT* p)>& processComponent) const
    {
        foreachUnderlying([](Component* p) -> void {
            processComponent(p->cast<UnderlyingComponentT>());
        });
    }
};

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(comp)
LM_NAMESPACE_BEGIN(detail)

// Type holder
template <typename... Ts>
struct TypeHolder {};

// Makes key for create function
template <typename T>
struct KeyGen {
    static std::string gen(const std::string& s) {
        // For ordinary type, use the given key as it is.
        return s;
    }
};
template <template <typename...> class T, typename... Ts>
struct KeyGen<T<Ts...>> {
    static std::string gen(const std::string& s) {
        // For template type, decorate the type with type list.
        return s + "<" + std::string(typeid(TypeHolder<Ts...>).name()) + ">";
    }
};

LM_NAMESPACE_END(detail)

// ----------------------------------------------------------------------------

/*!
    \brief Upcast/downcast of component types. 
    Use this class if the component instance can be nullptr.
*/
template <typename InterfaceT>
InterfaceT* cast(Component* p) {
    return p ? p->cast<InterfaceT>() : nullptr;
}

/*!
    \brief Create component with specific interface type.
    \tparam InterfaceT Component interface type.
    \param key Name of the implementation.
*/
template <typename InterfaceT>
Component::Ptr<InterfaceT> create(const std::string& key, Component* parent) {
    auto* inst = detail::createComp(
        detail::KeyGen<InterfaceT>::gen(std::move(key)).c_str(), parent);
    if (!inst) {
        return Component::Ptr<InterfaceT>(nullptr, nullptr);
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
Component::Ptr<InterfaceT> create(
    const std::string& key, Component* parent, const Json& prop)
{
    auto inst = create<InterfaceT>(key, parent);
    if (!inst || !inst->construct(prop)) {
        return Component::Ptr<InterfaceT>(nullptr, nullptr);
    }
    return inst;
}

/*
    \brief Given 'xxx.yyy.zzz', returns the pair of 'xxx' and 'yyy.zzz'.
*/
static std::tuple<std::string, std::string> splitFirst(const std::string& s) {
    const auto i = s.find_first_of('.', 0);
    if (i == std::string::npos) {
        return { s, "" };
    }
    return { s.substr(0, i), s.substr(i + 1) };
}

/*!
    \brief Get a given component or its underlying component based on name.
*/
static Component* getCurrentOrUnderlying(const std::string& r, Component* p) {
    return r.empty() ? p : p->underlying(r);
}

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(detail)

/*!
    \brief Create component instance.
    \param key Name of the implementation.
*/
LM_PUBLIC_API Component* createComp(const std::string& key, Component* parent);

// Register a component.
LM_PUBLIC_API void reg(
    const std::string& key,
    const Component::CreateFunction& createFunc,
    const Component::ReleaseFunction& releaseFunc);

// Unregister a component.
LM_PUBLIC_API void unreg(const std::string& key);

// Load a plugin.
LM_PUBLIC_API bool loadPlugin(const std::string& path);

// Load plugins inside a given directory.
LM_PUBLIC_API void loadPlugins(const std::string& directory);

// Unload loaded plugins.
LM_PUBLIC_API void unloadPlugins();

// ----------------------------------------------------------------------------

/*!
    \brief Create component instance directly with constructor.
    Use this function when you want to construct a component instance
    directly with the visibile component type.
*/
template <typename ComponentT, typename... Ts>
Component::Ptr<ComponentT> createDirect(Component* parent, Ts&&... args) {
    auto comp = std::make_unique<ComponentT>(std::forward<Ts>(args)...);
    comp->setParent(parent);
    return Component::Ptr<ComponentT>(
        comp.release(),
        [](Component* p) { LM_SAFE_DELETE(p); });
}

// ----------------------------------------------------------------------------

/*!
    \brief Singleton for a context component.
    This singleton holds the ownership the context component where
    we manages the component hierarchy under the context.
*/
template <typename ContextComponentT>
class ContextInstance {
private:
    Component::Ptr<ContextComponentT> context;

public:
    static ContextInstance& instance() {
        static ContextInstance instance;
        return instance;
    }

    static ContextComponentT& get() {
        return *instance().context.get();
    }

    static void init(const std::string& type, const Json& prop) {
        instance().context = comp::create<ContextComponentT>(type, nullptr, prop);
    }

    static void shutdown() {
        instance().context.reset();
    }
};

// ----------------------------------------------------------------------------

// Scope guard of `loadPlugins` and `loadPlugins`.
class ScopedLoadPlugin {
private:
    bool valid_ = true;

public:
    ScopedLoadPlugin(const std::string& path) : ScopedLoadPlugin({ path }) {}
    ScopedLoadPlugin(std::initializer_list<std::string> paths) {
        for (auto path : paths) {
            valid_ |= loadPlugin(path);
            if (!valid_) { break; }
        }
    }
    ~ScopedLoadPlugin() { unloadPlugins(); }
    LM_DISABLE_COPY_AND_MOVE(ScopedLoadPlugin)

public:
    bool valid() const { return valid_; }
};

// ----------------------------------------------------------------------------

// Helps registration of a component implementation.
template <typename ImplType>
class RegEntry {
private:
    std::string key_;

public:
    static RegEntry<ImplType>& instance(std::string key) {
        static RegEntry<ImplType> instance(KeyGen<ImplType>::gen(std::move(key)));
        return instance;
    }

public:
    RegEntry(std::string key) : key_(std::move(key)) {
        // Register factory function
        reg(key_.c_str(),
            []() -> Component* { return new ImplType; },
            [](Component* p)   { LM_SAFE_DELETE(p); });
    }
    ~RegEntry() {
        // Unregister factory function
        unreg(key_.c_str());
    }
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
#define LM_COMP_REG_IMPL(ImplT, Key) \
	namespace { \
		template <typename T> class RegEntry_Init_; \
		template <> class RegEntry_Init_<ImplT> { \
            static const LM_NAMESPACE::comp::detail::RegEntry<ImplT>& reg_; \
        }; \
        const LM_NAMESPACE::comp::detail::RegEntry<ImplT>& \
            RegEntry_Init_<ImplT>::reg_ = \
                LM_NAMESPACE::comp::detail::RegEntry<ImplT>::instance(Key); \
    }
