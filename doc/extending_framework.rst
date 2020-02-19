Extending framework
######################

In this section, we will explain how to extend the features of Lightmetrica.

.. note::

    This section focuses on describing the concepts rather than providing actual working codes to implement the extensions. For this, you can refer to :ref:`example`.

Component interface
==========================

Lightmetrica is designed to be extensible. The most of the features of the framework, e.g., materials, lights, cameras, or rendering techniques, can be implemented as extensions. This allows the developers to design flexible experiments according to their requirements.

Extension mechanism of Lightmetrica follows well-recognized paradigm of objected-oriented programming (OOP).
An extensible feature of the framework is provided as an abstract class (interface) of C++. 
To extend a feature, a developer wants to implement a class by inheriting the abstract class.

A typical extensible interface looks like the following code. This code example is taken from :cpp:class:`lm::Renderer` class defined in ``include/lm/renderer.h`` with some clean-ups. For other examples, you can open some headers in ``include/lm`` directory (e.g., ``light.h`` or ``material.h``). 

.. code-block:: cpp

    #include "component.h"

    LM_NAMESPACE_BEGIN(LM_NAMESPACE)

    class Renderer : public Component {
    public:
        virtual Json render() const = 0;
    };

    LM_NAMESPACE_END(LM_NAMESPACE)


As shown in the example, an extensible interface must inherit the common base class :cpp:class:`lm::Component`, which provides a necessary features as a class being managed by the object management system of the framework. Since an extensible interface always inherits :cpp:class:`lm::Component`, we call the extensible interface *component interface*. A component interface contains one or more virtual functions to be implemented by developers in a derived class. 

.. note::

    In the following discussion, all example codes are assumed to be defined inside ``lm`` namespace. 
    In the actual code, this can be done by enclosing the code by :c:func:`LM_NAMESPACE_BEGIN` and :c:func:`LM_NAMESPACE_END` macros by :c:macro:`LM_NAMESPACE`, which is resolved to ``lm``.

Configuring build environment
=============================

You want to follow the recommended practice described in :ref:`managing_experiments` according to your requirements of your experiment.

Implementing interface
==========================

Once you identify the component interface that you want to extend, the next step would be to implement your own class by inheriting the interface. For instance, the following code implements :cpp:class:`lm::Renderer` class, which just generates a blank image with the color given as a parameter.


.. code-block:: cpp

    #include <lm/renderer>
    // ...

    class Renderer_Blank final : public Renderer {
    private:
        Vec3 color_;
        Film* film_;

    public:
        virtual void construct(const Json& prop) override {
            color_ = json::value<Vec3>(prop, "color");
            film_ = json::comp_ref<Film>(prop, "output");
        }

        virtual Json render() const override {
            film_->clear();
            const auto [w, h] = film_->size();
            for (int y = 0; y < h; y++) {
                for (int x = 0; x < w; x++) {
                    film_->set_pixel(x, y, color_);
                }
            }
            return {};
        }
    };

    LM_COMP_REG_IMPL(Renderer_Blank, "renderer::blank");

You need to register the implemented class to the framework using :c:macro:`LM_COMP_REG_IMPL` macro, which takes the type of the implemented class and the identifier (in ``renderer::<name>`` format). The identifier is used to create the instance of the component from Python API. 

In the class we implement two functions. :cpp:func:`lm::Component::construct` function implements an function being called when the component instance is created. The function is a virtual function exposed in :cpp:class:`lm::Component` class. :cpp:func:`lm::Component::construct` function takes a parameter ``prop`` of ``lm::Json`` type, which is typically passed from Python API.

We can pass arbitrary parameters as long as the framework supports serialization of the type. 
We can get the values from ``prop`` using the API of `nlohmann/json`_ library, which is used to implement the feature, or the functions in ``lm::json`` namespace.

.. _`nlohmann/json`: https://github.com/nlohmann/json

In this example, :cpp:func:`lm::json::value` function checks ``color`` key in the given Json object. If the key is found, it tries to convert the underlying value to the type specified by the type parameter ``Vec3``. If the key is not found, or the type of the underlying value cannot be converted to ``Vec3``, the function throws an exception. :cpp:func:`json::comp_ref` function is used to get an instance of the other component using a given :ref:`asset locator <accessing_instance>`.

:cpp:func:`lm::Renderer::render` function implements the core logic of the renderering technique.
In this example, it just iterates through every pixel in the given film and set it to a constant color.

Using extended feature from Python API
----------------------------------------- 

In the case of :cpp:class:`lm::Renderer` class, the instance creation of the class corresponds to ``lm.load_renderer()`` function in Python API. The following code creates a renderer using the identifier (``blank``) which corresponds to the second half of the identifier used for the registration (``renderer::blank``).

.. code-block:: python

    renderer = lm.load_renderer('renderer', 'blank', {
        'color': [0,1,0],
        'output': film.loc(),
    })

.. note::

    For the usage of the Python API for assets loading, scene creation, or rendering, please refer to :ref:`basic_rendering`.

.. note::

    Actual usage of the implemented component depends on the interface. Many of the class representing scene assets (e.g., material, light, camera, etc.) can be created by ``lm.load_*`` function, but some interfaces uses different interface or even used implicitly in the framework. 
    For the full detail about the component object system of the framework, including the API to create an instance from C++ side, please refer to :ref:`Component`.
