.. _basic_rendering:

Basic rendering
######################

In this section, we will introduce the basic rendering feature of Lightmetrica. The topic includes initialization of the framework, loading and managing assets, creating scenes, and rendering. 

.. note::

    This section focuses on describing the concepts rather than providing actual working codes to render an image.
    For this, you can refer to :ref:`example`.

Lightmetrica API
==========================

Lightmetrica exposes API to the two programming languages: Python and C++. Python API is mainly used to organize the experiments. On the other hand, C++ API is mainly used to develop an extension. Both are not identical, but many of the API can be accessible from both environments.

In this guide, we will often use the references to the functions or classes of the framework. The references link to the corresponding entries in :ref:`api_ref`. We generate the API references based on C++ API, but the API is also exposed to Python if not specifically mentioned.

Readers are expected to convert the notations depending on the contexts. For instance, we use a link to :cpp:func:`lm::init` function to represent its Python-API variant ``lm.init()``.

.. note::

    Unlike previous versions, 
    our framework does not define our own scene definition file to describe the scene and assets.
    This is a design choice as a research-oriented renderer.
    On experiments, the scene is often used with parameters being determined programmatically.
    Even with scene definition file, we thus eventually need to introduce a layer to parameterize the scene definition file.
    In this version of the framework, we introduced comprehensive set of Python APIs,
    so we decided to use Python directly to configure the scene,
    making possible to completely remove a communication layer with scene definition file.

Importing framework
==========================

Assuming that we have finished build and project setup described in :ref:`build` and :ref:`managing_experimens`, we can start using Lightmetrica by importing ``lightmetrica`` module. In the following documentation, we will use ``lm`` as an alias of the ``lightmetrica`` module.

.. code-block:: python

    import lightmetrica as lm

When we are using the framework in Jupyter notebook, we can use ``lightmetrica_jupyter`` extension for some useful features. 

.. code-block:: python

    %load_ext lightmetrica_jupyter

Initialization
==========================

Initialization of framework
-------------------------------

We can initialize Lightmetrica with :cpp:func:`lm::init` function and shutdown with :cpp:func:`lm::shutdown` function. In case that you want to reset the internal state, you can use :cpp:func:`lm::reset` function. :cpp:func:`lm::init` function takes optional configuration as an argument.

.. code-block:: python

    lm.init()

If the initialization is successful, you can show the information of the framework using :cpp:func:`lm::info` function.

.. code-block:: python

    lm.info()

Initialization of subsystems
-------------------------------

A *subsystem* refers to globally accessible features of the framework, which for instance includes logging, progress reporting, or parallel computation. The related API of a subsystem is exposed under ``lm.<name_of_subsystem>`` namespace. For convenience, :cpp:func:`lm::init` function initializes various subsystems with default settings. 

We can reconfigure the default settings with ``lm.<name_of_subsystem>.init()`` function. For instance, logging subsystem (``log``) can be initialized with :cpp:func:`lm::log::init` function. Here, we initialize ``log`` and ``progress`` subsystem for Jupyter notebook.

.. code-block:: python

    lm.log.init('jupyter')
    lm.progress.init('jupyter')

.. note::

    The internal state of the subsystem are not refreshed by :cpp:func:`lm::reset` function.
    The function only clears the loaded assets.

.. _preparing_asset:

Loading asset
===============================

Using ``lm.load_*()`` function
-------------------------------

*Asset* represents a component of the scene such as meshes or materials.
To load an asset, you can use ``lm.load_<asset_type>()`` function, where ``<asset_type>`` is the name of the asset interface. For instance, ``film`` asset can be loaded by ``lm.load_film()`` function.

.. code-block:: python

    film = lm.load_film('film1', 'bitmap', {
        'w', 1920,
        'h', 1080
    });

``lm.load_<asset_type>()`` function takes three arguments. The first argument specifies the identifier of the asset, which is used to reference the asset internally. The second argument is the detailed name of the asset of the interface creating interface. The third argument is the parameter to initialize the asset.

In this example, we want to make ``film`` asset. This function takes the name of the asset (``film1``) and the type of the asset (``bitmap``) of the creating interface.

The return value of the function is a reference to the interface type of the asset. For instance, ``lm.load_film()`` function returns a reference to :cpp:class:`lm::Film` class.  Using the reference, you can access the underlying member functions. For detail, please refer to :ref:`api_ref`.

If the asset with the same identifier is already registered,
the function try to reload the assets and replace the old one.

Using ``lm.load()`` function
-------------------------------

Alternatively, we can use a general ``lm.load()`` function to load an asset. The arguments are almost same, but the type of the asset must contain the interface name (``film``) and the separator (``::``). This function is useful when you want to create an asset of user-defined interface type.

.. code-block:: python

    film_base = lm.asset('film1', 'film::bitmap', {
        'w': 1920,
        'h': 1080
    })

Note that ``lm.asset()`` function returns an instance of :cpp:class:`lm::Component` class, not the interface type of the asset (e.g., :cpp:class:`lm::Film`). 
:cpp:class:`lm::Component` is a base type of all assets. If you want to access the members of the specific type, you want to use ``.cast()`` function of the target interface.

.. code-block:: python

    film = lm.Film.cast(film_base)

.. note::

    For convenience, we will sometimes refer to a pair of asset interface and detailed asset type by ``interface::name`` format.
    For instance, ``film::bitmap`` or ``material::diffuse`` etc.

.. _accessing_instance:

Accessing instance
===============================

The created instance is internally managed by the framework. This means the lifetime of the instance is not tied to the lifetime of the python object (e.g., ``film1`` variable). It is merely a reference to the instance internally managed in the framework.

In addition to using the instance being returned by ``lm.load_<asset_type>()`` function, you can use *component locator* to access the instance. A component locator is a string starting with the character ``$`` and the words connected by ``.``. A locator indicates a location of the instance managed by the framework. 
For instance, the locator of the ``film1`` asset is ``$.assets.film1``. This can be obtained by ``.loc()`` function.

.. code-block:: python

    film.loc()

You can obtain the instance of the asset by the locator. ``lm.get_<asset_type>()`` function takes the locator as an argument and returns the instance. For instance, the following code gets the same instance as ``film1``.

.. code-block:: python

    film = lm.get_film('$.assets.film1')

Similarly, the general ``lm.get()`` function returns an instance of :cpp:class:`lm::Component` class, similarly to ``lm.create_asset`` function. You thus need to cast the type before use.

.. code-block:: python

    film = lm.Film.cast(lm.get('$.assets.film1'))

.. note::

    The assets managed by the framework can be printed using ``lm.debug.print_asset_tree()`` function.

Passing instance as a parameter
===============================

When we create an asset by ``lm.load_<asset_type>()`` function, we can pass an reference to the other asset as a parameter. For instance, ``material::diffuse`` takes a reference to a texture representing the diffuse reflectance of the material. You can pass the reference to the asset with the locator. 

.. code-block:: python

    texture = lm.load_texture('texture_constant', 'constant', {
        'color': [1,1,1]
    })
    material = lm.load_material('material_diffuse', 'diffuse', {
        'mapKd': texture.loc()
        # or directly
        # 'mapKd': '$.assets.texture_constant'
    })

.. _making_scene:

Scene setup using primitives
======================================

*Scene* represents a collection of objects to be rendered. A scene of Lightmetrica can be viewed as a collection of *primitives*. A primitive is an element of the scene which associates mesh or material etc. with transformation.

Creating scene asset
--------------------------------------

A scene is a special asset. We can thus create the asset by ``lm.load_scene()`` function. The second argument is fixed to ``default``.

.. code-block:: python

    scene = lm.load_scene('scene', 'default', {})

Practically, a scene requires *acceleration structure* (interface type: ``accel``). Since ``accel`` is also an asset, it can be created by ``lm.load_accel()`` function.

.. code-block:: python

    accel = lm.load_accel('accel', 'sahbvh', {})
    scene = lm.load_scene('scene', 'default', {
        'accel': accel.loc()
    })

Creating a primitive
--------------------------------------

Once a scene is loaded, you can create primitives using :cpp:func:`lm::Scene::add_primitive` function.
For instance, the following code creates a primitive associating ``mesh1`` and ``material1`` assets. 

.. code-block:: python

    scene.add_primitive({
        'mesh': mesh.loc(),
        'material': material.loc()
    })

Creating a primitive with transformation
-----------------------------------------

Additionally, you can use :cpp:func:`lm::Scene::add_transformed_primitive` function to specify the transformation applied to the geometry. A transformation can be specified by 4x4 matrix.
Specifically, by setting the transformation being identity matrix, the following code is equivalent to the above.

.. code-block:: python

    scene.add_transformed_primitive(lm.identity(), {
        'mesh': mesh.loc(),
        'material': material.loc()
    })

We can also create a primitive not being associated with ``mesh``, like ``camera``:

.. code-block:: python

    scene.add_primitive({
        'camera': camera.loc()
    })

Using primitive generator
--------------------------------------

Some assets like ``model`` works as a *primitive generator*.
If such an asset is specified, :cpp:func:`lm::Scene::add_primitive` function internally generates multiple multiple primitives and add them to the scene. 
If a transformation is specified in this case, the same transformation is applied to all the primitives generated.

.. code-block:: python

    scene.add_primitive({
        'model': model.loc()
    })
    
Scene setup using scene graph
======================================

An advanced approach to configure a scene is to use *scene graph*. A scene graph can describe the relationship of the transformed primitives using tree-like structure (precisely, DAG: directed acyclic graph).

Node types
--------------------------------------

The scene graph of Lightmetrica has three different node types: (1) primitive node, (2) transform group node, (3) instance group node. 

(1) A *primitive node* describes the association of geometry and material. In a scene graph, this node serves as a leaf node. 

(2) A *transform group* describes the local transformation applied to the child nodes. The global transformation applied to a leaf node (primitive) by computing a product of local transformation stored in this node type. 

(3) A *instance group* is a special group node to specify the child nodes are used as an instanced geometry. This information is used by some acceleration structures to reduce the memory footprint by reusing the acceleration structures for the underlying geometry. Unlike other types of nodes, this node type can have multiple parents.

Creating a node
--------------------------------------

:cpp:class:`lm::Scene` provides an interface to create a scene graph nodes.
We can create each node type by (1) :cpp:func:`lm::Scene::create_primitive_node`, (2) :cpp:func:`lm::Scene::create_group_node`, (3) :cpp:func:`lm::Scene::create_instance_group_node` respectively.
For detail, please visit the corresponding API reference.

All these functions returns an integer index of the node. Scene graph nodes are managed inside a scene instance. The interfaces of the scene class allows the user to access or manipulate the scene graph structure using the index. 

.. note::

    The root node of the scene graph is fixed to a transform group with identity transformation and its node index is fixed to zero. We can get the node index of the root node (=0) using :cpp:func:`lm::Scene::root_node` function.

Adding a node to scene graph
--------------------------------------

Note that a newly created node is *floating*, because the node is not yet added to the scene graph. This means we need to connect the created node to an existing node in the scene graph.

We can add a child node to the existing node in the scene graph with :cpp:func:`lm::Scene::add_child` function, where the first argument is the index of the node to add a new node, and the second argument being the index of the node being added. For instance, the following snippet creates primitive (``p``) and transformation group (``t``) nodes, where (1) ``p`` is added to ``t``, and (2) ``t`` is added to the root node.

.. code-block:: python

    p = scene.create_primitive_node({
        'mesh': mesh.loc(),
        'material': material.loc()
    })
    t = scene.create_group_node(<transformation_matrix>)
    scene.add_child(t, p)                   # (1)
    scene.add_child(scene.root_node(), t)   # (2)
    
Building acceleration structure
======================================

If we loaded the scene with acceleration structure,
we need to execute the post-process to construct the acceleration structure
after the completion of the scene setup.
This can be done by calling :cpp:func:`lm::Scene::build` function. 

.. code-block:: python

    scene.build()

.. note::

    :cpp:func:`lm::Scene::build` function must be called whenever after the structural modification happens to the scene.

Rendering
======================================

We need a *renderer* asset to render an image. The renderer asset can be loaded using ``lm.load_renderer()`` function.
Typically, a renderer takes ``scene`` asset to be rendered and ``film`` into which the renderer outputs. 
For instance, if you want to load ``renderer::raycast`` asset, you will write:

.. code-block:: python

    renderer = lm.load_renderer('renderer', 'raycast', {
        'scene': scene.loc(),
        'output': film.loc(),
        # additional option to configure renderer::raycast
    })

You can execute the rendering process by calling :cpp:func:`lm::Renderer::render` function. 
Once finished, the rendered image will be written into the specified ``film``.

.. code-block:: python

    renderer.render()

Checking result
======================================

Saving rendered image
--------------------------------------

You can save the rendered image using :cpp:func:`lm::Film::save` function.
The function takes a path to the output image as an argument. 
The output type is automatically inferred by the extension of the filename.
The supported image format depends on the using ``film`` asset type.
For instance, ``film::bitmap`` supports some typical formats like ``.hdr``, ``.png``, etc.

.. code-block:: python

    film.save('<image_path>')

Displaying rendered image
--------------------------------------

When we are using Jupyter notebook, it is useful to directly visualize the rendered image in the same notebook using ``matplotlib``.

To do this, we need to aceess the underlying image data and feed it to the library.
The underlying image data of ``film`` can be obtained by :cpp:func:`lm::Film::buffer` function.
For convenience, the image data can be accessed as a ``numpy.array``, which can be directly feed into ``.imshow()`` function.

.. code-block:: python

    img = np.copy(film.buffer())
    f = plt.figure(figsize=(15,15))
    ax = f.add_subplot(111)
    ax.imshow(np.clip(np.power(img,1/2.2),0,1), origin='lower')
    plt.show()

.. note::

    :cpp:func:`lm::Film::buffer` function does not make a copy of the internal image data. If the internal state changes, for instance when you dispatch the renderer again, the buffer becomes invalid. You thus want to explicitly copy the buffer if you need to use it afterwards, e.g., with ``np.copy()`` function.

.. note::

    The image data obtained with :cpp:func:`lm::Film::buffer` function is not gamma corrected.
    You want to apply gamma correction before the image is visualized.