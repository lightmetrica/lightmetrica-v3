Component
######################

Overview
===========================

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
We can create an instance of a compoment by :cpp:func:`lm::comp::create` function.
For instance, creating ``testcomponent::impl`` component reads

.. code-block:: cpp

    const auto testcomp = lm::comp::create<TestComponent>("testcomponent::impl", "");

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

The process is achieved by implementing :cpp:func:`lm::Component::underlying` function. Example:

.. code-block:: cpp

    struct TestComponent_QueryLocator final : public TestComponent {
        Ptr<lm::Component> comp;
        virtual Component* underlying(const std::string& name) const override {
            const auto [s, r] = lm::comp::splitFirst(name);
            if (s == "comp") {
                return lm::comp::getCurrentOrUnderlying(r, comp.get());
            }
            return nullptr;
        }
        virtual bool construct(const lm::Json& prop) override {
            comp = ...
            return true;
        }
    }

Each component need to implement 1) separating the first part of the locator and the remaining part,
2) selection of the underlying component, 3) determine and return the selected component pointer thereof, or underlying component of the selected components.

In the example, given the input ``name="comp.some.thing"``, (1) :cpp:func:`lm::comp::splitFirst` function first separates the string into ``s="assets"`` and ``r="some.thing"``, and (2) if the first part of the locator matches ``s == "comp"``, (3) the function returns corresponding component ``comp.get()`` or its underlying components. The determination process is automatically done with :cpp:func:`lm::comp::getCurrentOrUnderlying` function.

Querying information
===========================

A component provides a way to query underlying components
and the framework utilizes this feature to implement some advanced features.


Supporting serialization
===========================

Singleton
===========================

Python binding
===========================
