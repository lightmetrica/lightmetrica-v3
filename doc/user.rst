Basic rendering
######################

*User context* subsystem provides user-friendly wrapper for the high-level manipulation of the assets, scenes, or rendering. The users want to use this subsystem to implement their experiments. 
Due to the frequent use, the functions of this subsystem is exposed just below ``lm`` namespace. 

Initialization
======================

User context subsystem can be initialized with :cpp:func:`lm::init` function
and shutdown with :cpp:func:`lm::shutdown` function.
In case that you want to reset the internal state, you can use :cpp:func:`lm::reset` function.
:cpp:func:`lm::init` function takes user context name (default: ``user::default``) in the first argument
and the configuration in the second argument.

For convenience, user context subsystem automatically initializes various subsystems
with default types. You can reconfigure the default subsystems with corresponding ``init`` functions.
For instance, if you want to change the logger subsystem after initialization, you can do

.. code-block:: cpp

    lm::init();
    lm::log::init("logger::yourlogger", { ... });

The underlying component implementing user context subsystem is automatically registered
as a root node of the component hierarchy.

.. note::

    :cpp:func:`lm::info` prints information about the framework.

.. _preparing_asset:

Preparing asset
======================

*Asset* represents a component of the scene such as meshes or materials.
User context provides a set of APIs to load and manage assets.
To load an asset, you can use :cpp:func:`lm::asset` function, like

.. code-block:: cpp

    lm::asset("film1", "film::bitmap", {
        {"w", 1920},
        {"h", 1080}
    });

This invocation create an asset called ``film1`` of the type ``film::bitmap``,
with two parameters ``w`` and ``h``.
The parameters are used as an argument to call :cpp:func:`lm::Component::construct` of the corresponding component implementation.
If the asset with the same identifier is already registered,
the function try to reload the assets and replace the old one.

Although this example uses a predefined component, in fact, any component implementation including user-defined component can be an asset.
This means you can make your own assets even if the framework does not provide an interface for that.

:cpp:func:`lm::asset` function returns component locator (see :ref:`component_hierarchy_and_locator`) which can be used to get a weak reference to the asset inside the component hierarchy.

.. code-block:: cpp

    const auto loc = lm::asset("film1", ...);
    const auto* film = lm::comp::get<Film>(loc);

Once you load an asset, you can also use an overload of :cpp:func:`lm::asset` function
to get the locator of the asset from the identifier.
The function is useful when an asset requires to 
pass locator of the another asset as a parameter.

.. code-block:: cpp

    lm::asset("film1", "film::bitmap", { ... });
    lm::asset("camera1", "camera::pinhole", {
        {"film", lm::asset("film1")},
        ...
    });

Note that in this example, even when you want to replace ``film1``,
you don't need to update a reference inside ``camera1``.
Our framework automatically finds weak references inside the component hierarchy pointing to the old component 
and replaces with the reference to the new component, as long as the component properly enumerates the underlying components (see :ref:`querying_information`).

.. _making_scene:

Making scene
======================

A scene of Lightmetrica consists of a set of *primitives*.
A *primitive* is an element of the scene which associates a mesh and a material with transformation.
To create a primitive, you can use :cpp:func:`lm::primitive` function.
For instance,

.. code-block:: cpp

    lm::primitive(lm::Mat4(1), {
        {"mesh", lm::asset("mesh1")},
        {"material", lm::asset("material1")}
    });

creates a primitive associating ``mesh1`` and ``material1`` assets predefined before.
The first argument is the transformation. Here we specify identity matrix.
You can also create a primitive not associated with a mesh, like camera:

.. code-block:: cpp

    lm::primitive(lm::Mat4(1), {
        {"camera", lm::asset("camera1")}
    });

A certain asset like ``model`` works as a *primitive generator*.
If a primitive generator is specified, :cpp:func:`lm::primitive` function creates multiple primitives.
In this case, if a transformation is specified, the same transformation is applied to all the primitives generated.

.. code-block:: cpp

    lm::primitive(lm::Mat4(1), {
        {"model", lm::asset("obj1")}
    });

.. note::

    Unlike previous versions, 
    our framework does not define our own scene definition file to describe the scene and assets.
    This is a design choice as a research-oriented renderer.
    On experiments, the scene is often used with parameters being determined programmatically.
    Even with scene definition file, we thus eventually need to introduce a layer to parameterize the scene definition file.
    In this version of the framework, we introduced comprehensive set of Python APIs,
    so we decided to use Python directly to configure the scene,
    making possible to completely remove a communication layer with scene definition file.
    
Rendering
======================

Once we setup the scene, we are ready for rendering.
The steps for rendering is twofold: (1) building acceleration structure, (2) dispatching rendering.

First, you can build the acceleration structure by :cpp:func:`lm::build` function.
You can specify the type of the acceleration structure in the first argument.
The second argument is the configuration parameters for the acceleration structure.
For instance, the following invocation build the acceleration structure with ``accel::sahbvh``
using default parameters.

.. code-block:: cpp

    lm::build("accel::sahbvh");

Now you can dispatch rendering with :cpp:func:`lm::render` function.
The first argument is the renderer type and the second argument is the configuration parameters for the renderer.

.. code-block:: cpp

    lm::render("renderer::raycast", {
        {"output", lm::asset("film1")},
        ...
    });

The above version of :cpp:func:`lm::render` function creates an instance of renderer components every time
the function is called. This is not efficient if you need frequent invocations.
Instead, you can separate the call with initialization and dispatch.
:cpp:func:`lm::renderer` function configures the renderer, and an overloaded version of :cpp:func:`lm::render` function dispatches the rendering using the configuration.

.. code-block:: cpp

    lm::renderer("renderer::raycast", { ... });
    lm::render();
    ...


Saving image
======================

You can save the rendered image using :cpp:func:`lm::save` function conveniently.
The function takes the locator of a film as first argument, and image path as second argument.

.. code-block:: cpp

    lm::save(lm::asset("film"), "<image path>");

When you want to access the underlying image buffer directly, you can use :cpp:func:`lm::buffer` function.
This function is useful when you want to feed the image data to another library to visualize the rendered image.

.. code-block:: cpp

    const auto buf = lm::buffer(lm::asset("film"));

Note that :cpp:func:`lm::buffer` function does not make a copy of the internal image data.
Thus if the internal state changes, for instance when you dispatch the renderer again, the buffer becomes invalid.
You want to explicitly copy the buffer if you need to use it afterwards.