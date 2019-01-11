/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "common.h"
#include <any>
#include <memory>
#include <string>
#include <optional>
#include <functional>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \addtogroup comp
    @{
*/

// ----------------------------------------------------------------------------

/*!
    \brief Base component.

    Base class of all components in Lightmetrica.
    All components interfaces and implementations must inherit this class.
*/
class Component {
private:
    friend struct comp::detail::Access;
    friend struct ComponentDeleter;

public:
    //! Factory function type
    using CreateFunction  = std::function<Component*()>;

    //! Release function type
    using ReleaseFunction = std::function<void(Component*)>;

    //! Unique pointer for component instances.
    template <typename InterfaceT>
    using Ptr = std::unique_ptr<InterfaceT, ComponentDeleter>;

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
    LM_DISABLE_COPY_AND_MOVE(Component)

public:
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
    virtual bool construct(const Json& prop) { LM_UNUSED(prop); return true; }
    
    /*!
        \brief Deserialize a component.
    */
    virtual void load(InputArchive& ar) { LM_UNUSED(ar); }

    /*!
        \brief Serialize a component.
    */
    virtual void save(OutputArchive& ar) const { LM_UNUSED(ar); }
    
public:
    /*!
        \brief Get underlying component instance.
    */
    virtual Component* underlying(const std::string& name = "") const { LM_UNUSED(name); return nullptr; }

    /*!
        \brief Get underlying component instance with specific interface type.
    */
    template <typename UnderlyingComponentT>
    UnderlyingComponentT* underlying(const std::string& name = "") const {
        // Use dynamic_cast directly instead of cast() function
        // to handle the case with nullptr.
        return dynamic_cast<UnderlyingComponentT*>(underlying(name));
    }

    /*!
        \brief Get underlying component instance by index.
    */
    virtual Component* underlyingAt(int index) const { LM_UNUSED(index); return nullptr; }

    /*!
        \brief Get underlying component instance by index with specifc interface type.
    */
    template <typename UnderlyingComponentT>
    UnderlyingComponentT* underlyingAt(int index) const {
        return dynamic_cast<UnderlyingComponentT*>(underlyingAt(index));
    }

    /*!
        \brief Get underlying comonent by property.
        If there is no matching entry, return nullptr.
    */
    Component* underlying(const Json& prop, const std::string& name) const {
        const auto it = prop.find(name);
        if (it == prop.end()) {
            // No entry
            return nullptr;
        }
        if (it->is_string()) {
            return underlying(*it);
        }
        else if (it->is_number()) {
            return underlyingAt(*it);
        }
        // Invalid type
        return nullptr;
    }

    /*!
        \brief Get underlying comonent by property with specific interface type.
    */
    template <typename UnderlyingComponentT>
    UnderlyingComponentT* underlying(const Json& prop, const std::string& name) const {
        return dynamic_cast<UnderlyingComponentT*>(underlying(prop, name));
    }

    /*!
        \brief Process given function for each underlying component call.
    */
    virtual void foreachUnderlying(const std::function<void(Component* p)>& processComponent) const {
        LM_UNUSED(processComponent);
    }

    /*!
        \brief Process given function for each underlying component call with specific interface type. 
    */
    template <typename UnderlyingComponentT>
    void foreachUnderlying(
        const std::function<void(UnderlyingComponentT* p)>& processComponent) const
    {
        foreachUnderlying([&](Component* p) -> void {
            processComponent(p->cast<UnderlyingComponentT>());
        });
    }
};

// ----------------------------------------------------------------------------

/*!
    \brief Deleter for component instances
    This must be default constructable because pybind11 library
    requires to construct unique_ptr from a single pointer.
*/
struct ComponentDeleter {
    ComponentDeleter() = default;
    void operator()(Component* p) const noexcept {
        if (auto releaseFunc = p->releaseFunc_; releaseFunc) {
            // If the instance is directly created inside Python script
            // and managed by Python interpreter, releaseFunc can be nullptr. 
            releaseFunc(p);
        }
    }
};

// ----------------------------------------------------------------------------

/*!
    @}
*/

// We are using old style of nested namespace in headers
// because doxygen doesn't support nested namespace in C++17.
LM_NAMESPACE_BEGIN(comp)
LM_NAMESPACE_BEGIN(detail)

// Accessor for private members of Component
struct Access {
    LM_DISABLE_CONSTRUCT(Access)
    static std::string& key(Component* p) { return p->key_; }
    static const std::string& key(const Component* p) { return p->key_; }
    static Component::CreateFunction& createFunc(Component* p) { return p->createFunc_; }
    static const Component::CreateFunction& createFunc(const Component* p) { return p->createFunc_; }
    static Component::ReleaseFunction& releaseFunc(Component* p) { return p->releaseFunc_; }
    static const Component::ReleaseFunction& releaseFunc(const Component* p) { return p->releaseFunc_; }
    static std::any& ownerRef(Component* p) { return p->ownerRef_; }
    static const std::any& ownerRef(const Component* p) { return p->ownerRef_; }
};

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

/*!
    \addtogroup comp
    @{
*/

/*!
    \brief Create component instance.
    \param key Name of the implementation.
*/
LM_PUBLIC_API Component* createComp(const std::string& key);

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

// Iterate registered component names.
LM_PUBLIC_API void foreachRegistered(const std::function<void(const std::string& name)>& func);

// Print registered component names.
LM_PUBLIC_API void printRegistered();

/*!
    @}
*/

LM_NAMESPACE_END(detail)

// ----------------------------------------------------------------------------

/*!
    \addtogroup comp
    @{
*/

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
Component::Ptr<InterfaceT> create(const std::string& key) {
    auto* inst = detail::createComp(
        detail::KeyGen<InterfaceT>::gen(std::move(key)).c_str());
    if (!inst) {
        return {};
    }
    return Component::Ptr<InterfaceT>(dynamic_cast<InterfaceT*>(inst));
}

/*!
    \brief Create component with construction with given properties.
    \tparam InterfaceT Component interface type.
    \param key Name of the implementation.
    \param prop Properties.
*/
template <typename InterfaceT>
Component::Ptr<InterfaceT> create(const std::string& key, const Json& prop) {
    auto inst = create<InterfaceT>(key);
    if (!inst || !inst->construct(prop)) {
        return {};
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

/*!
    @}
*/

LM_NAMESPACE_BEGIN(detail)

/*!
    \addtogroup comp
    @{
*/

/*!
    \brief Create component instance directly with constructor.
    Use this function when you want to construct a component instance
    directly with the visibile component type.
*/
template <typename ComponentT, typename... Ts>
Component::Ptr<ComponentT> createDirect(Ts&&... args) {
    auto comp = std::make_unique<ComponentT>(std::forward<Ts>(args)...);
    return Component::Ptr<ComponentT>(comp.release());
}

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
        instance().context = comp::create<ContextComponentT>(type, prop);
    }

    static void shutdown() {
        instance().context.reset();
    }

    // Check if the context instance is initialized
    static bool initialized() {
        return bool(instance().context);
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
            []() -> Component* {
                return new ImplType;
            },
            [](Component* p) {
                LM_SAFE_DELETE(p);
            });
    }
    ~RegEntry() {
        // Unregister factory function
        unreg(key_.c_str());
    }
};

/*!
    @}
*/

LM_NAMESPACE_END(detail)
LM_NAMESPACE_END(comp)
LM_NAMESPACE_END(LM_NAMESPACE)

// ----------------------------------------------------------------------------

/*!
    \addtogroup comp
    @{
*/

/*!
    \brief Register implementation.
    \param ImplT Component implemenation type.
    \param Key Name of the implementation.

    \rst
    This macro registers an implementation of a component object into the framework.
    This macro can be placed under any translation unit irrespective to the kind of binaries it belongs,
    like a shared libraries or an user's application code.

    .. note::
       According to the C++ specification `[basic.start.dynamic]/5`_, dependening on the implementation, 
       the dynamic initalization of an object inside a namespace scope `can be defered`
       until it is used in the context that its definition must be presented (odr-used).
       This macro workarounded this issue by expliciting instantiating an singleton ``RegEntry<>``
       defined for each implementation type.

       .. _[basic.start.dynamic]/5: http://eel.is/c++draft/basic#start.dynamic-5
    \endrst
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

/*!
    @}
*/