/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "common.h"
#include "exception.h"
#include "jsontype.h"
#include "serialtype.h"
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

// ------------------------------------------------------------------------------------------------

/*!
    \brief Base component.

    \rst
    Base class of all components in Lightmetrica.
    All components interfaces and implementations must inherit this class.
    For detail, see :ref:`component`.
    \endrst
*/
class Component {
private:
    friend struct comp::detail::Access;
    friend struct ComponentDeleter;

public:
    /*!
        \brief Factory function type.
        \return Component instance.
        
        \rst
        A type of the function to create an component instance.
        The function of this type is registered automatically to the framework
        with :c:func:`LM_COMP_REG_IMPL` macro.
        \endrst
    */
    using CreateFunction = std::function<Component*()>;

    /*!
        \brief Release function type.
        \param p Component instance to be deleted.

        \rst
        A type of the function to release an component instance.
        The function of this type is registered automatically to the framework
        with :c:func:`LM_COMP_REG_IMPL` macro.
        \endrst
    */
    using ReleaseFunction = std::function<void(Component* p)>;

    /*!
        \brief Unique pointer for component instances.
        \tparam InterfaceT Component interface type.

        \rst
        unique_ptr for component types.
        All component instances must be managed by unique_ptr,
        because we constrained an instance inside our component hierarchy
        must be managed by a single component.
        \endrst
    */
    template <typename InterfaceT>
    using Ptr = std::unique_ptr<InterfaceT, ComponentDeleter>;

    /*!
        \brief Visitor function type.
        \param p Visiting component instance.
        \param weak True if the visiting instance is referred as a weak reference.

        \rst
        The function of this type is used to be a callback function of
        :cpp:func:`lm::Component::foreachUnderlying` function.
        \endrst
    */
    using ComponentVisitor = std::function<void(Component*& p, bool weak)>;

private:
    //! Name of the component instance.
    std::string key_;

    //! Global locator of this component if accessible.
    std::string loc_;

    //! Factory function of the component instance.
    CreateFunction create_func_ = nullptr;

    //! Release function of the component instance.
    ReleaseFunction release_func_ = nullptr;

    //! Underlying reference to owner object (if any)
    std::any owner_ref_;

public:
    Component() = default;
    virtual ~Component() = default;
    LM_DISABLE_COPY_AND_MOVE(Component)

public:
    /*!
        \brief Get name of component instance.

        \rst
        This function returns an unique identifier
        of the component implementation of the instance.
        \endrst
    */
    const std::string& key() const { return key_; }

    /*!
        \brief Get locator of the component.

        \rst
        If the instance is integrated into the component hierarchy,
        this function returns the unique component locator of the instance.
        If the instance is not in the component hierarchy,
        the function returns an empty string.
        \endrst
    */
    const std::string& loc() const {
        return loc_;
    }

    /*!
        \brief Get parent locator.
        
        \rst
        This function returns parent locator of the component.
        If this or parent component is root, the function returns empty string.
        For instance, if the current locator is ``aaa.bbb.ccc``, this function returns ``aaa.bbb``.
        If the current locator is ``aaa``, this function returns empty string.
        \endrst
    */
    const std::string parent_loc() const {
        const auto i = loc().find_last_of('.');
        if (i == std::string::npos) {
            return "";
        }
        return loc().substr(0, i);
    }

    /*!
        \brief Get name of the component instance.

        \rst
        This fuction returns name of the component instance
        used as an identifier in parent component.
        \endrst
    */
    const std::string name() const {
        const auto i = loc().find_last_of('.');
        assert(i != std::string::npos);
        return loc().substr(i + 1);
    }

    /*!
        \brief Make locator by appending string.
        \param base Base locator.
        \param child Child locator.

        \rst
        This function appends the specified ``child`` locator
        into the back of ``base`` locator.
        \endrst
    */
    const std::string make_loc(const std::string& base, const std::string& child) const {
        assert(!base.empty());
        assert(!child.empty());
        return base + "." + child;
    }

    /*!
        \brief Make locator by appending string to current locator.
        \param child Child locator.

        \rst
        This function appends the specified locator into the back of
        the locator of the current instance.
        \endrst
    */
    const std::string make_loc(const std::string& child) const {
        return make_loc(loc(), child);
    }

public:
    /*!
        \brief Construct a component.
        \param prop Configuration property.
        
        \rst
        The function is called just after the component instance is created.
        Mostly, the function is called internally by :cpp:func:`lm::comp::create` function.
        The configuration properties are passed by JSON type.
        \endrst
    */
    virtual void construct(const Json& prop) { LM_UNUSED(prop); }
    
    /*!
        \brief Deserialize a component.
        \param ar Input archive.

        \rst
        The function deserialize the component using the archive ``ar``.
        The function is called internally. The users do not want to use this directly.
        \endrst
    */
    virtual void load(InputArchive& ar) {
        LM_UNUSED(ar);
    }

    /*!
        \brief Serialize a component.
        \param ar Output archive.

        \rst
        The function serialize the component using the archive ``ar``.
        The function is called internally. The users do not want to use this directly.
        \endrst
    */
    virtual void save(OutputArchive& ar) {
        LM_UNUSED(ar);
    }

public:
    /*!
        \brief Get underlying component instance.
        \param name Name of the component instance being queried.

        \rst
        This function queries a component instance by the identifier ``name``.
        If the component instance is found, the function returns a pointer to the instance.
        If not component instance is found, the function returns nullptr.

        The name of the component instance found by this function must match ``name``,
        that is, ``comp->name() == name``.
        \endrst
    */
    virtual Component* underlying(const std::string& name) const { LM_UNUSED(name); return nullptr; }

    /*!
        \brief Process given function for each underlying component call.
        \param visitor Component visitor.

        \rst
        This function enumerates underlying component of the instance.
        Every time a component instance (unique or weak) is found,
        the callback function specified by an argument is called.
        The callback function takes a flag to show the instance is weak reference or unique.
        Use :cpp:func:`lm::comp::visit` function to automatically determine either of them.
        \endrst
    */
    virtual void foreach_underlying(const ComponentVisitor& visitor) { LM_UNUSED(visitor); }

    /*!
        \brief Get underlying value.
        \param query Query string.

        \rst
        This function gets underlying values of the component.
        The specification of the query string is implementation-dependent.
        The return type must be serialized to Json type.
        \endrst
    */
    virtual Json underlying_value(const std::string& query = "") const { LM_UNUSED(query); return {}; }

    /*!
        \brief Get underlying raw pointer.
        \param query Query string.

        \rst
        This function gets underlying content as a void pointer.
        We want to use this function for debugging purposes.
        \endrst
    */
    virtual void* underlying_raw_pointer(const std::string& query = "") const { LM_UNUSED(query); return nullptr; }
};

// ------------------------------------------------------------------------------------------------

/*!
    \brief Deleter for component instances

    \rst
    This must be default constructable because pybind11 library
    requires to construct unique_ptr from a single pointer.
    \endrst
*/
struct ComponentDeleter {
    ComponentDeleter() = default;
    void operator()(Component* p) const noexcept {
        if (auto releaseFunc = p->release_func_; releaseFunc) {
            // If the instance is directly created inside Python script
            // and managed by Python interpreter, releaseFunc can be nullptr. 
            releaseFunc(p);
        }
    }
};

// ------------------------------------------------------------------------------------------------

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
    static std::string& loc(Component* p) { return p->loc_; }
    static const std::string& loc(const Component* p) { return p->loc_; }
    static Component::CreateFunction& createFunc(Component* p) { return p->create_func_; }
    static const Component::CreateFunction& createFunc(const Component* p) { return p->create_func_; }
    static Component::ReleaseFunction& releaseFunc(Component* p) { return p->release_func_; }
    static const Component::ReleaseFunction& releaseFunc(const Component* p) { return p->release_func_; }
    static std::any& ownerRef(Component* p) { return p->owner_ref_; }
    static const std::any& ownerRef(const Component* p) { return p->owner_ref_; }
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
    \param key Implementation key.

    \rst
    This function creates a component instance from the key.
    The component implementation with the key must be registered beforehand
    with :c:func:`LM_COMP_REG_IMPL` macro and loaded with :cpp:func:`load_plugin` function
    if the component is defined inside a plugin.
    Otherwise the function returns nullptr with error message.
    \endrst
*/
LM_PUBLIC_API Component* create_comp(const std::string& key);

/*!
    \brief Register a component.
    \param key Implementation key.
    \param createFunc Create function.
    \param releaseFunc Release function.

    \rst
    This function registers a component implementation into the framework.
    The function is internally and indirectly called by :c:func:`LM_COMP_REG_IMPL` macro.
    The users do not want to use it directly.
    \endrst
*/
LM_PUBLIC_API void reg(
    const std::string& key,
    const Component::CreateFunction& createFunc,
    const Component::ReleaseFunction& releaseFunc);

/*!
    \brief Unregister a component.
    \param key Implementation key.

    \rst
    This function unregisters a component implementation specified by the key.
    The users do not want to use it directly.
    \endrst
*/
LM_PUBLIC_API void unreg(const std::string& key);

/*!
    \brief Load a plugin.
    \param path Path to a plugin.
    
    \rst
    This function loads a plugin from a specified path.
    The components inside the plugin are automatically registered to the framework
    and ready to use with :cpp:func:`lm::comp::create` function.
    If the loading fails, the function returns false.
    \endrst
*/
LM_PUBLIC_API bool load_plugin(const std::string& path);

/*!
    \brief Load plugins inside a given directory.
    \param directory Path to a directory containing plugins.

    \rst
    This functions loads all plugins inside the specified directory.
    If the loading fails, it generates an error message but ignored.
    \endrst
*/
LM_PUBLIC_API void load_plugin_directory(const std::string& directory);

/*!
    \brief Unload loaded plugins.
    
    \rst
    This functions unloads all the plugins loaded so far.
    \endrst
*/
LM_PUBLIC_API void unload_plugins();

/*!
    \brief Iterate registered component names.
    \param func Function called for each registered component.

    \rst
    This function enumerates registered component names.
    The specified callback function is called for each registered component.
    \endrst
*/
LM_PUBLIC_API void foreach_registered(const std::function<void(const std::string& name)>& func);

/*!
    \brief Register root component.
    \param p Component to be registered as a root.

    \rst
    This function registers the given component as a root component.
    The registered component is used as a starting point of the component hierarchy.
    The given component must have a locator ``$``.
    The function is called internaly so the user do not want to use it directly.
    \endrst
*/
LM_PUBLIC_API void register_root_comp(Component* p);

//! \cond
LM_PUBLIC_API Component* get(const std::string& locator);
//! \endcond

/*!
    @}
*/

LM_NAMESPACE_END(detail)

// ------------------------------------------------------------------------------------------------

/*!
    \addtogroup comp
    @{
*/

using detail::load_plugin;
using detail::load_plugin_directory;
using detail::unload_plugins;
using detail::foreach_registered;

/*!
    \brief Get component by locator.
    \tparam T Component interface type.
    \param locator Component locator.

    \rst
    This function queries an underlying component instance inside the component hierarchy
    by a component locator. For detail of component locator, see :ref:`component_hierarchy_and_locator`.
    If a component instance is not found, or the locator is ill-formed,
    the function returns nullptr with error messages.
    \endrst
*/
template <typename T>
T* get(const std::string& locator) {
    return dynamic_cast<T*>(detail::get(locator));
}

/*!
    \brief Update weak reference.
    \param p Reference to the component pointer.

    \rst
    Helper function to update weak reference of component instance.
    \endrst
*/
template <typename T>
std::enable_if_t<std::is_base_of_v<lm::Component, T>, void>
updateWeakRef(T*& p) {
    if (!p) {
        return;
    }
    const auto loc = p->loc();
    if (loc.empty()) {
        return;
    }
    p = comp::get<T>(loc);
}

/*!
    \brief Visit underlying asset overloaded for weak references.
    \param visitor Visitor function.
    \param p Reference to component pointer.
*/
template <typename T>
std::enable_if_t<std::is_base_of_v<lm::Component, T>, void>
visit(const Component::ComponentVisitor& visitor, T*& p) {
    Component* temp = p;
    visitor(temp, true);
    p = dynamic_cast<T*>(temp);
}

/*!
    \brief Visit underlying asset overloaded for unique pointers.
    \param visitor Visitor function.
    \param p Reference to component pointer.
*/
template <typename T>
std::enable_if_t<std::is_base_of_v<lm::Component, T>, void>
visit(const Component::ComponentVisitor& visitor, Component::Ptr<T>& p) {
    Component* temp = p.get();
    visitor(temp, false);
}

/*!
    \brief Create component with specific interface type without calling construct function.
    \tparam InterfaceT Component interface type.
    \param key Name of the implementation.
    \param loc Component locator of the instance.
*/
template <typename InterfaceT>
Component::Ptr<InterfaceT> create_without_construct(const std::string& key, const std::string& loc) {
    auto* inst = detail::create_comp(
        detail::KeyGen<InterfaceT>::gen(std::move(key)).c_str());
    if (!inst) {
        return {};
    }
    detail::Access::loc(inst) = loc;
    return Component::Ptr<InterfaceT>(dynamic_cast<InterfaceT*>(inst));
}

/*!
    \brief Create component with construction with given properties.
    \tparam InterfaceT Component interface type.
    \param key Name of the implementation.
    \param loc Component locator of the instance.
    \param prop Properties.
*/
template <typename InterfaceT>
Component::Ptr<InterfaceT> create(const std::string& key, const std::string& loc, const Json& prop = {}) {
    auto inst = create_without_construct<InterfaceT>(key, loc);
    if (!inst) {
        return {};
    }
    inst->construct(prop);
    return inst;
}

// ------------------------------------------------------------------------------------------------

/*!
    @}
*/

LM_NAMESPACE_BEGIN(detail)

/*!
    \addtogroup comp
    @{
*/

/*!
    \brief Singleton for a context component.
    \tparam ContextComponentT Component type.

    \rst
    This singleton holds the ownership the context component where
    we manages the component hierarchy under the context.
    Example:

    .. code-block:: cpp

        using Instance = lm::comp::detail::ContextInstance<YourComponentInterface>;

        Instance::init("interface::yourcomponent", { ... });
        Instance::get()->...
    \endrst
*/
template <typename ContextComponentT>
class ContextInstance {
private:
    Component::Ptr<ContextComponentT> context;

public:
    ~ContextInstance() {
        shutdown();
    }

public:
    static ContextInstance& instance() {
        static ContextInstance instance;
        return instance;
    }

    /*!
        \brief Get a reference to the underlying component.
        \return Reference to the underlying component.
    */
    static ContextComponentT& get() {
        if (!initialized()) {
            LM_THROW_EXCEPTION(Error::Uninitialized,
                "Uninitialized global component. Possible failure to call *::init() function.");
        }
        return *instance().context.get();
    }

    /*!
        \brief Initialize the underlying component.
        \param type Component type.
        \param prop Configuration property.

        \rst
        This function initializes the underlying component
        with the specified component type and properties.
        \endrst
    */
    static void init(const std::string& type, const Json& prop) {
        // Implicitly call shutdown() if the singleton was already initialized
        if (instance().context) {
            shutdown();
        }
        // Instance of the context component has root locator ($)
        instance().context = comp::create<ContextComponentT>(type, "$", prop);
    }

    /*!
        \brief Delete the underlying component.
    */
    static void shutdown() {
        instance().context.reset();
    }

    /*!
        \brief Check if the context instance is initialized.
        
        \rst
        This function returns true if the underlying component is initialized.
        \endrst
    */
    static bool initialized() {
        return bool(instance().context);
    }
};

// ------------------------------------------------------------------------------------------------

/*!
    \brief Scope guard of `loadPlugins` and `loadPlugins`.
*/
class ScopedLoadPlugin {
private:
    bool valid_ = true;

public:
    ScopedLoadPlugin(const std::string& path) : ScopedLoadPlugin({ path }) {}
    ScopedLoadPlugin(std::initializer_list<std::string> paths) {
        for (auto path : paths) {
            valid_ |= load_plugin(path);
            if (!valid_) { break; }
        }
    }
    ~ScopedLoadPlugin() { unload_plugins(); }
    LM_DISABLE_COPY_AND_MOVE(ScopedLoadPlugin)

public:
    bool valid() const { return valid_; }
};

// ------------------------------------------------------------------------------------------------

/*!
    \brief Registration entry for component implementation.
    \tparam ImplType Type of component implementation.

    \rst
    This class is used internally by :c:func:`LM_COMP_REG_IMPL` macro.
    The users do not want to use it directly.
    \endrst
*/
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

// ------------------------------------------------------------------------------------------------

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
    See :ref:`implementing_interface` for detail.

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