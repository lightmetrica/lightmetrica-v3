Tutorial
############

This section describes the usage of the framework by various examples.
As we do not provide standalone renderer executable, all rendering process must be described with C++ or Python APIs.
We provided working codes for the corresponding sections of the tutorials. 

We refer to the functions by the convension of C++ APIs if both C++ and Python APIs are provided, e.g., :func:`lm::init`. Some examples or APIs are specific either to C++ or Python. Those will be explicitly specified if necessary.

.. ----------------------------------------------------------------------------

Rendering blank image
=============

- C++:  `example/blank.cpp`_
- Python: `example/blank.py`_

.. _example/blank.cpp: https://github.com/hi2p-perim/lightmetrica-v3/blob/master/example/blank.cpp
.. _example/blank.py: https://github.com/hi2p-perim/lightmetrica-v3/blob/master/example/blank.py

Let's start to use Lightmetrica by rendering a blank image:


.. tabs::

   .. group-tab:: C++

      .. literalinclude:: ../example/blank.cpp
         :language: cpp
         :start-after: // _begin_example
         :end-before: // _end_example

   .. group-tab:: Python

      .. literalinclude:: ../example/blank.py
         :language: python
         :start-after: # _begin_example
         :end-before: # _end_example

For C++, the first line includes everything necessary to use Lightmetrica.
For Python, we import the ``lightmetrica`` module, where we use ``lm`` as an alias of ``lightmetrica`` for simplicity.


.. tabs::

   .. group-tab:: C++

      .. literalinclude:: ../example/blank.cpp
         :language: cpp
         :lines: 7

   .. group-tab:: Python

      .. literalinclude:: ../example/blank.py
         :language: python
         :lines: 4

After including header, you can initialize the framwork by calling :func:`lm::init` function. You can pass various arguments to configure the framework to the funciton, but here we keep it empty so that everything is configured to be default.

.. tabs::

   .. group-tab:: C++

      .. literalinclude:: ../example/blank.cpp
         :language: cpp
         :lines: 11-12
         :dedent: 8

   .. group-tab:: Python

      .. literalinclude:: ../example/blank.py
         :language: python
         :lines: 6-7

Next we define `assets` necessary to dispatch renderer, like materials, meshes, etc. In this example, we only need a `film` to which the renderer outputs the image. We can define assets by :func:`lm::asset` function. The first argument (``film``) specifies the name of the asset to be referenced. The second argument (``film::bitmap``) is given as the type of the assets formatted as ``<type of asset>::<implementation>`` where the last argument (``{...}``) specifies the parameters passed to the instance. ``film::bitmap`` takes two parameters ``w`` and ``h`` which respectively specify width and height of the film.

.. tabs::

   .. group-tab:: C++

      .. literalinclude:: ../example/blank.cpp
         :language: cpp
         :lines: 14-16
         :dedent: 8

   .. group-tab:: Python

      .. literalinclude:: ../example/blank.py
         :language: python
         :lines: 9-11

Now we are ready for rendering. :func:`lm::render` function dispatches rendering where we speficy type of the renderer and the parameters as arguments. ``renderer::blank`` is a toy renderer that only produces a blank image to the film specifies by ``film`` parameter where we can use predefined ID of the asset. Also, we can specify the background color by ``color`` parameter.

.. tabs::

   .. group-tab:: C++

      .. literalinclude:: ../example/blank.cpp
         :language: cpp
         :lines: 18-22
         :dedent: 8

   .. group-tab:: Python

      .. literalinclude:: ../example/blank.py
         :language: python
         :lines: 13-17

After rendering, the generated image is kept in ``film``. :func:`lm::save` function outputs this film to the disk as an image.

.. tabs::

   .. group-tab:: C++

      .. literalinclude:: ../example/blank.cpp
         :language: cpp
         :lines: 24-25
         :dedent: 8

   .. group-tab:: Python

      .. literalinclude:: ../example/blank.py
         :language: python
         :lines: 19-20

For C++, we can additionally put exception handling code. When an unrecoverable error happens, Lightmetrica produces an exception of ``std::runtime_error`` containing the description useful for debugging. 
:func:`LM_ERROR` macro is an convenience macro to produce error message through logger system. 

.. literalinclude:: ../example/blank.cpp
   :language: cpp
   :lines: 27-29
   :dedent: 4

Executing the code will produce the following image.
Note that the image was converted to JPG image from PFM.

.. figure:: _static/example/blank.jpg

.. ----------------------------------------------------------------------------

Rendering quad
=============

- C++:  `example/quad.cpp`_
- Python: `example/quad.py`_

.. _example/quad.cpp: https://github.com/hi2p-perim/lightmetrica-v3/blob/master/example/quad.cpp
.. _example/quad.py: https://github.com/hi2p-perim/lightmetrica-v3/blob/master/example/quad.py

This section describes how to render a simple scene containing a quad represented by two triangles. From this example we do not provide the full source code inside this page. Please follow the files inside ``example`` directory if necessary.

The code starts again with :func:`lm::init` function. Yet in this time, we specify the number of threads used for parallel processes by ``numThread`` parameter. The negative number configures the number of threads deducted from the maximum number of threads. For instance, if we specify ``-1`` on the machine with the maximum number of threads ``32``, the function configures the number of threads by ``31``. 

.. tabs::

   .. group-tab:: C++

      .. literalinclude:: ../example/quad.cpp
         :language: cpp
         :start-after: // _begin_init
         :end-before: // _end_init
         :dedent: 8

   .. group-tab:: Python

      .. literalinclude:: ../example/quad.py
         :language: python
         :start-after: # _begin_init
         :end-before: # _end_init

Next we define the assets. In addition to ``film``, we define ``camera``, ``mesh``, and ``material``. Although the types of assets are different, we can use consistent interface to define the assets. Here we prepare for a pinhole camera (``camera::pinhole``), a raw mesh (``mesh::raw``), and a diffuse material (``material::diffuse``) with the corrsponding parameters. Please refer to the corresponding pages for the detailed description of the parameters.

.. tabs::

   .. group-tab:: C++

      .. literalinclude:: ../example/quad.cpp
         :language: cpp
         :start-after: // _begin_assets
         :end-before: // _end_assets
         :dedent: 8

   .. group-tab:: Python

      .. literalinclude:: ../example/quad.py
         :language: python
         :start-after: # _begin_assets
         :end-before: # _end_assets

The scene of Lightmetrica is defined by a set of ``primitives``. A primitive specifies an object inside the scene by associating geometries and materials with transformation. We can define a primitive by :func:`lm::primitive` function where we specifies transformation matrix and associating assets as arguments.
In this example we define two pritimives; one for camera and the other for quad mesh with diffuse material. Transformation is given by 4x4 matrix. Here we specified identify matrix meaning no transformation.

.. tabs::

   .. group-tab:: C++

      .. literalinclude:: ../example/quad.cpp
         :language: cpp
         :start-after: // _begin_primitive
         :end-before: // _end_primitive
         :dedent: 8

   .. group-tab:: Python

      .. literalinclude:: ../example/quad.py
         :language: python
         :start-after: # _begin_primitive
         :end-before: # _end_primitive

For this example we used ``renderer::raycast`` for rendering. 
This renderer internally uses acceleration structure for ray-scene intersections. 
The acceleration structure can be given by :func:`lm::build` function. In this example we used ``accel::sahbvh``.

.. tabs::

   .. group-tab:: C++

      .. literalinclude:: ../example/quad.cpp
         :language: cpp
         :start-after: // _begin_render
         :end-before: // _end_render
         :dedent: 8

   .. group-tab:: Python

      .. literalinclude:: ../example/quad.py
         :language: python
         :start-after: # _begin_render
         :end-before: # _end_render

Executing the code will produce the following image.

.. figure:: _static/example/quad.jpg

.. ----------------------------------------------------------------------------

Raycasting a scene with OBJ models
=============

- C++:  `example/raycast.cpp`_
- Python: `example/raycast.py`_

.. _example/raycast.cpp: https://github.com/hi2p-perim/lightmetrica-v3/blob/master/example/raycast.cpp
.. _example/raycast.py: https://github.com/hi2p-perim/lightmetrica-v3/blob/master/example/raycast.py

This example demonstrates how to render a scene with OBJ models using raycasting. In order to support parametrized rendering by the command line arguments, we provided a simple helper function :func:`lm::parsePositionalArgs` to parse positional arguments with specified values as a JSON format. 

.. tabs::

   .. group-tab:: C++

      .. literalinclude:: ../example/raycast.cpp
         :language: cpp
         :start-after: // _begin_parse_args
         :end-before: // _end_parse_args
         :dedent: 8

   .. group-tab:: Python

      .. literalinclude:: ../example/raycast.py
         :language: python
         :start-after: # _begin_parse_args
         :end-before: # _end_parse_args

Following is the definition of assets. We used the aforementioned command line arguments to parametrize the assets. To load an OBJ model, we can use ``model::wavefrontobj`` asset. This asset internally creates meshes and materials by reading the associated MTL file.


.. tabs::

   .. group-tab:: C++

      .. literalinclude:: ../example/raycast.cpp
         :language: cpp
         :start-after: // _begin_asset
         :end-before: // _end_asset
         :dedent: 8

   .. group-tab:: Python

      .. literalinclude:: ../example/raycast.py
         :language: python
         :start-after: # _begin_asset
         :end-before: # _end_asset

We can create primitives from the loaded ``model::wavefrontobj`` asset by using :func:`lm::primitives` function. 

.. tabs::

   .. group-tab:: C++

      .. literalinclude:: ../example/raycast.cpp
         :language: cpp
         :start-after: // _begin_primitives
         :end-before: // _end_primitives
         :dedent: 8

   .. group-tab:: Python

      .. literalinclude:: ../example/raycast.py
         :language: python
         :start-after: # _begin_primitives
         :end-before: # _end_primitives

Executing the renderer will produce the following image.
For command line parameters, please refer to ``example/run_all.py``.

.. figure:: _static/example/raycast_fireplace_room.jpg

   Scene: fireplace_room

.. figure:: _static/example/raycast_cornell_box.jpg

   Scene: cornell_box_sphere

.. ----------------------------------------------------------------------------

Rendering with path tracing
=============

- C++:  `example/pt.cpp`_
- Python: `example/pt.py`_

.. _example/pt.cpp: https://github.com/hi2p-perim/lightmetrica-v3/blob/master/example/pt.cpp
.. _example/pt.py: https://github.com/hi2p-perim/lightmetrica-v3/blob/master/example/pt.py

This section describes how to render the scene with path tracing. Path tracing is a rendering technique based on Monte Carlo method and notably one of the most basic (yet practical) rendering algorithms taking global illumination into account. Our framework implements path tracing as ``renderer::pt`` renderer. 

The use of the renderer is straightforward; we just need to specify ``renderer::pt`` with :func:`lm::render` function with some renderer-specific parameters. Thanks to the modular design of the framework, the most of the code can be the same as the case of ray casting. 

.. tabs::

   .. group-tab:: C++

      .. literalinclude:: ../example/pt.cpp
         :language: cpp
         :start-after: // _begin_render
         :end-before: // _end_render
         :dedent: 8

   .. group-tab:: Python

      .. literalinclude:: ../example/pt.py
         :language: python
         :start-after: # _begin_render
         :end-before: # _end_render

Rendered images:

.. figure:: _static/example/pt_fireplace_room.jpg

   Scene: fireplace_room

.. figure:: _static/example/pt_cornell_box.jpg

   Scene: cornell_box_sphere

