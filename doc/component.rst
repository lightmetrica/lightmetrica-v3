.. _component:

Component
######################

Lightmetrica is build upon a component object system
which provides various features to support extensibility as well as usability
based on object-oriented paradigm.
All extensible features of the framework, e.g., materials or renderers etc., are implemented based on this system.

The purpose of our component object system is to provide a complete decoupling
between the interface and the implementation. 
Our component interface is based on the type erasure by standard virtual classes.
Once an instance is created, the type of the implementation is erased and
we can access the underlying implementation through the interface type.

This is good, but a problem is in the creation of the instance.
To create an instance, we need to know the type of the derived type
which increase the coupling between the creator of the instance.
Also, we need to exposed the header of the derived classes.
This means it increase the coupling to the private members of the class
(without tricks like pimpl).

To avoid this, a common practice is to use abstract factory.
Instead of using actual type of the derived class,
an instance is created by an (string) identifier.
The factory need to know the mapping between the identifier and the way of creating instance of an implementation.
Our component object system can automate the registration process by a simple macro.
Furthermore, our component system is flexible enough to implement plugins
as easily as built-in components.

Creating interface
===========================

A *component interface* is simply an C++ virtual class that directly/indirectly inherits :cpp:class:`lm::Component` class.
Note that :cpp:class:`lm::Component` can also be a component interface.
An example of the component interface with a virtual function would be like:

.. code-block:: cpp

    struct TestComponent : public lm::Component {
       virtual int f() = 0;
    };

.. _implementing_interface:

Implementing interface
===========================

*Implementing* a component interface is same as C++ standard in a sense that
the user needs to inherit an interface type and override the virtual functions.
The implemented class must be registered to the framework via :c:func:`LM_COMP_REG_IMPL` macro.
For instance, implementing ``TestComponent`` above looks like

.. code-block:: cpp

    struct TestComponent_Impl final : public TestComponent {
        virtual int f() override { ... }
    }

    LM_COMP_REG_IMPL(TestComponent_Impl, "testcomponent::impl");

:c:func:`LM_COMP_REG_IMPL` macro takes a class name in the first argument and the identifier of the class in the second argument.
The identifier is just an string and we can specify any letters in it, yet as a convention, we used ``interface::impl`` format throughout the framework. 

.. note::

    The registration process happens in static initialization phase.
    If an identifier is already registered, the framework reports an runtime error through standard output.

.. note::

    You can check registered implementations by :cpp:func:`lm::comp::foreachRegistered` function.

Creating plugin
===========================

Creating plugins of the framework is straightforward.
A plugin is just an component implementation placed in the dynamically loadable context.
This is flexible because the user do not need to care about the change of the syntax to create an plugin.
An dynamic libraries can contain as many component implementation as you want.
A plugin can be loaded by :cpp:func:`lm::comp::loadPlugin` function
and unloaded by :cpp:func:`lm::comp::unloadPlugins` function.

Creating instance
===========================

Once a registration has done, we are ready to use it.
We can create an instance of a component by :cpp:func:`lm::comp::create` function.
For instance, creating ``testcomponent::impl`` component reads

.. code-block:: cpp

    const auto comp = lm::comp::create<TestComponent>("testcomponent::impl", "");

The first argument is the identifier of the implementation,
the second argument is the component locator of the instance if the object is integrated into the global component hierarchy.
For now, let's keep it empty. You need to specify the type of the component interface with template type. 
If the instance creation fails, the function will return nullptr.

:cpp:func:`lm::comp::create` function returns unique_ptr of the specified interface type.
The lifetime management of the instance is up to the users.
The unique_ptr is equipped with a custom deleter to support the case where the instance is created in the different dynamic libraries.

Parameterized creation
===========================

We can pass arbitrary arguments in a JSON-like format as a third argument of :cpp:func:`lm::comp::create` function.
We depend `nlohmann/json`_ library to achieve this feature. See the link for the supported syntax and types.

.. _`nlohmann/json`: https://github.com/nlohmann/json

.. code-block:: cpp

    const auto testcomp = lm::comp::create<TestComponent>("testcomponent::impl_with_param", "", {
        {"param1", 42},
        {"param2", "hello"}
    });

The parameters are routed to :cpp:func:`lm::Component::construct` function
implemented in the specified component. We can extract the values from the Json type
using accessors like STL containers.

.. code-block:: cpp

    struct TestComponent_ImplWithParam final : public TestComponent {
        virtual bool construct(const lm::Json& prop) override {
            const int param1 = prop["param1"];
            const std::string param2 = prop["param2"];
            ...
            return true;
        }
        virtual int f() override { ... }
    }

    LM_COMP_REG_IMPL(TestComponent_ImplWithParam, "testcomponent::impl_with_param");

.. note::

    For convenience, we provided serializers
    to automatically convert types to/from the JSON type,
    which includes e.g. vector / matrix types, raw pointer types.

.. _component_hierarchy_and_locator:

Component hierarchy and locator
===============================

Composition of the unique_ptr of components or raw pointers inside a component implicitly defines
a *component hierarchy* of the components.
In the framework, we adopts a strict ownership constraint
that one instance of the component can only be possessed and managed by a single another component.
In other words, we do not allow to use shared_ptr to manage the instance of the framework. 
This constraint makes it possible to identify a component inside the hierarchy by a locator.

A *component locator* is a string to uniquely identify an component instance inside the hierarchy. 
The string start with the character ``$`` and arbitrary sequence of characters separated by ``.``.
For instance, ``$.assets.obj1.mesh1``. Each string separated by ``.`` is used to identify the components
owned by the current node inside the hierarchy. By iteratively tracing down the hierarchy from the root,
the locator can identify an single component instance.

When we create an instance, we can also specify the component locator in the second argument.
An helper function :cpp:func:`lm::Component::makeLoc` is useful to make locator appending to the current locator.
For instance, the following creation of an instance called inside :cpp:func:`lm::Component::construct` function of a component with locator ``$.test``
will create a component with locator ``$.test.test2``.

.. code-block:: cpp

    struct TestComponent_Container final : public lm::Component {
        Ptr<lm::Component> comp;
        virtual bool construct(const lm::Json& prop) override {
            // Called inside a component with locator = $.test,
            // create an instance with locator = $.test.comp
            comp = lm::comp::create<lm::Component>("testcomponent::nested", makeLoc("comp"));
            return true;
        }
        virtual Component* underlying(const std::string& name) const override {
            // Underlying component must be accessible with the same name specified in create function
            return name == comp->name() ? comp.get() : nullptr;
        }
    };

Also, the underlying component must be accessible by the specified name using :cpp:func:`lm::Component::underlying` function.
:cpp:func:`lm::Component::name` function is useful to extract the name of the component.
Once the above setup is complete, we can access the underlying component globally by :cpp:func:`lm::comp::get` function.

.. code-block:: cpp

   const auto comp = lm::comp::get<lm::Component>("$.test.comp");

.. note::

    Some advanced features like serialization are based on this mechanism.
    Even if it seems to be working without ill-formed components, e.g., 
    those not specifying locator or not implementing :cpp:func:`lm::Component::underlying` function,
    it will definitely break some feature in the end.

.. note::

    A root component is internally configured and the user do not care about it.
    But for instance for testing purpose, we can configure it using :cpp:func:`lm::comp::detail::registerRootComp` function.
    The default root component is :cpp:class:`lm::user::detail::UserContext`.

Weak references
===========================

A raw pointer composed inside a component is handled as a weak reference to the other (owned) components.
Our framework only allows weak reference as a back edge (the edge making cycles) in the component hierarchy.
Weak references are often used by being injected to the other components
using :cpp:func:`lm::Component::construct` function.

For instance, the following component accepts ``ref`` parameter as a string
representing the locator of the component.
We can then inject the weak reference using :cpp:func:`lm::comp::get` function.

.. code-block:: cpp

    struct TestComponent_WeakRef1 final : public lm::Component {
        lm::Component* ref;
        virtual bool construct(const lm::Json& prop) override {
            ref = lm::comp::get<lm::Component>(prop["ref"]);
            return true;
        }
    };

Alternatively, one can inject the raw pointer directly to the component.
because the pointer types are automatically serialized to JSON type.
This strategy is especially useful when we want to inject the pointer of the type
inaccessible from the component hierarchy.

.. code-block:: cpp

    const lm::Component* ref = ...
    const auto comp = lm::comp::create<lm::Component>("testcomponent::weakref2", "", {
        {"ref", ref}
    });

.. code-block:: cpp

    struct TestComponent_WeakRef2 final : public lm::Component {
        lm::Component* ref;
        virtual bool construct(const lm::Json& prop) override {
            ref = prop["ref"];
            return true;
        }
    };

.. _querying_information:

Querying information
===========================

A component provides a way to query underlying components
and the framework utilizes this feature to implement some advanced features.
Every component with underlying components must implement the following functions: :cpp:func:`lm::Component::underlying`
and :cpp:func:`lm::Component::foreachUnderlying`.

:cpp:func:`lm::Component::underlying` function return the component with a query by name.
:cpp:func:`lm::Component::foreachUnderlying` function on the other hands enumerates all the underlying components.
``visit`` function needs to distinguish both unique_ptr (owned pointer) and raw pointer (weak reference) in the second argument. Yet :cpp:func:`lm::comp::visit` function will call them automatically according to the types for you.


.. code-block:: cpp

    struct TestComponent_Container1 final : public lm::Component {
        std::vector<Ptr<lm::Component>> comps;
        std::unordered_map<std::string, int> compMap;
        virtual Component* underlying(const std::string& name) const override {
            return comp.at(compMap.at(name)).get();
        }
        virtual void foreachUnderlying(const ComponentVisitor& visit) override {
            for (auto& comp : comps) {
                lm::comp::visit(visit, comp);
            }
        }
    };

.. code-block:: cpp

    struct TestComponent_Container2 final : public lm::Component {
        lm::Component* ref1;
        lm::Component* ref2;
        virtual Component* underlying(const std::string& name) const override {
            if (name == "ref1") { return ref1; }
            if (name == "ref2") { return ref2; }
            return nullptr;
        }
        virtual void foreachUnderlying(const ComponentVisitor& visit) override {
            lm::comp::visit(visit, ref1);
            lm::comp::visit(visit, ref2);
        }
    };

Supporting serialization
===========================

Our serialization feature depends on `cereal`_ library.
Yet unfortunately, a polymorphism support of cereal library is restricted because
the declaration of the derived class must be exposed to the global.
In our component object system, an implementation is completely separated from the interface
and there is no way to find corresponding implementation automatically.

.. _`cereal`: https://github.com/USCiLab/cereal

We workaround this issue by using providing two virtual functions: :cpp:func:`lm::Component::save` and :cpp:func:`lm::Component::load` to implement serialization for a specific archive,
and route the object finding mechanism of cereal to use these functions.
This means we can no longer use arbitrary archive type.
The default archive type is defined as ``lm::InputArchive`` and ``lm::OutputArchive``.

Implementing almost-similar two virtual functions are cumbersome.
To mitigate this, we provided :c:func:`LM_SERIALIZE_IMPL` helper macro.
The following code serializes member variables including component instances, or weak references.
Note that we can even serialize raw pointers, as long as they are weak references pointing to 
a component inside the component tree, and accessible by component locator. 

.. code-block:: cpp

    struct TestComponent_Serial final : public lm::Component {
        int v;
        std::vector<Ptr<lm::Component>> comp;
        lm::Component* ref;
        LM_SERIALIZE_IMPL(ar) {
            ar(v, comp, ref);
        }
    };


Singleton
===========================

A component can be used as a singleton,
and our framework implemented globally-accessible yet extensible features using component as singleton.
For convenience, we provide :cpp:class:`lm::comp::detail::ContextInstance` class to
make any component interface a singleton. 

.. Python binding
.. ===========================
