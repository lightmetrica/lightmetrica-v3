Build
############

This section describes build process of Lightmetrica.

.. ----------------------------------------------------------------------------

Prerequisites
=============

We need CMake_ (>= 3.10) and Python_ (>=3) to build and use the framework.
Optionally, we need doxygen_ to generate documentation.

.. _CMake: https://cmake.org/
.. _Python: https://www.python.org/
.. _doxygen: http://www.doxygen.nl/

.. ----------------------------------------------------------------------------

Installing dependencies
==========================

First you want to setup the external dependencies.
The external dependencies of the framework is all header-only libraries.
We suppose two strategies to introduce dependencies based on whether to use 1) in-project directory or 2) pre-installed packages.

Using in-project directory
--------------------------

This strategy uses in-project ``external`` directory to manage dependencies. To use this strategy, you want to create ``external`` directory in the project root and download all dependencies into the directory.
The build script automatically uses the dependencies if it finds ``external`` directory in the project root.
The following commands illustrates the steps.

.. code-block:: bash

   $ cd external
   $ git clone --depth 1 git@github.com:pybind/pybind11.git
   $ git clone --depth 1 git@github.com:nlohmann/json.git
   $ git clone --depth 1 git@github.com:g-truc/glm.git
   $ git clone --depth 1 git@github.com:onqtam/doctest.git
   $ git clone --depth 1 git@github.com:fmtlib/fmt.git
   $ git clone --depth 1 git@github.com:USCiLab/cereal.git

.. note::
   This strategy internally uses on CMake's ``add_subdirectory`` to find dependencies.

Using pre-installed packages
----------------------------

To use this strategy, you need to install the dependencies externally
using package management system or install the libraries from sources.
For instance, our `Dockerfile`_ adopts this strategy.
Please refer to the file for the detailed procedure.

.. _Dockerfile: https://github.com/hi2p-perim/lightmetrica-v3/blob/master/Dockerfile

.. note::
   This strategy internally uses on CMake's ``find_package`` to find dependencies.
   Plus, Lightmetrica's CMake script always tries to find packages with `config mode`_.
   Unfortunately, some libraries did not expose well-defined configuration files. This may result to a failure in configuration step when we execute the cmake command.

   .. _config mode: https://cmake.org/cmake/help/latest/command/find_package.html#full-signature-and-config-mode

.. note::
   This strategy is mandatory when Lightmetrica is used as a library and integrated into the user's CMakefile with ``find_package`` command. This is because Lightmetrica's configuration script recursively calls ``find_package`` command to resolve transitive dependencies.

.. ----------------------------------------------------------------------------

Building the framework
==========================

Windows
-------------

Tested with Visual Studio 2017 Version 15.9.
You can generate solution for Visual Studio with the following commands.
To build Python binding, be sure to activate the previously-created Python environment and start Visual Studio from the same shell.

.. code-block:: bash

   $ mkdir build && cd build
   $ cmake -G "Visual Studio 15 2017 Win64" ..
   $ start lightmetrica.sln


Linux
-------------

Tested with GCC 8.3 and `Ninja`_. The following commands generates the binaries under ``build/bin`` directory.

.. _Ninja: https://ninja-build.org/

.. code-block:: bash

   $ mkdir build && cd build
   $ cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release ..
   $ ninja

Additionally, execute the following command to install Lightmetrica to your system. If you want to change installation directory, add ``-DCMAKE_INSTALL_PREFIX=<install dir>`` to the ``cmake`` command.

.. code-block:: bash

   $ ninja install

.. ----------------------------------------------------------------------------

Using Lightmetrica as external library
=======================================

To use Lightmetrica as external library, you need to 
configure Lightmetrica as a dependency inside your own ``CMakeLists.txt``.
We again have two approaches, whether to use ``add_subdirectory`` or ``find_package``.

Using add_subdirectory
-------------

The first approach directly includes Lightmetrica's source directory via ``add_subdirectory``. You can use both options in :ref:`Installing dependencies` for the transitive dependencies. 
The following ``CMakeLists.txt`` shows minimum example of this approach. 
Once you include the directory, you can use ``lightmetrica::liblm`` target to link main library to your application.

.. code-block:: cmake
    :emphasize-lines: 3

    cmake_minimum_required(VERSION 3.10)
    project(your_renderer)
    add_subdirectory(lightmetrica)
    add_executable(your_renderer "your_renderer.cpp")
    target_link_libraries(your_renderer PRIVATE lightmetrica::liblm)

Using find_package
-------------

The second approach uses ``find_package`` with config-file mode to find a dependency to Lightmetrica. 
To use this approach, we need to use second option to install the dependencies, because the transitive dependencies must be also searchable via ``find_package``. 
Please find `example/ext`_ directory where we build some examples externally using Lightmetrica.

.. _`example/ext`: https://github.com/hi2p-perim/lightmetrica-v3/blob/master/example/ext/CMakeLists.txt

.. code-block:: cmake
    :emphasize-lines: 3

    cmake_minimum_required(VERSION 3.10)
    project(your_renderer)
    find_package(lightmetrica REQUIRED)
    add_executable(your_renderer "your_renderer.cpp")
    target_link_libraries(your_renderer PRIVATE lightmetrica::liblm)

.. note::

   When the configuation for Lightmetrica is not located in `standard search locations`_, we need explicitly add ``-Dlightmetrica_DIR=<install dir>`` option to your ``cmake`` command. 

   .. _standard search locations: https://cmake.org/cmake/help/latest/command/find_package.html#search-procedure

.. ----------------------------------------------------------------------------

Editing documentation
==========================

Install dependencies

.. code-block:: bash

   $ conda install -c conda-forge sphinx
   $ pip install sphinx-autobuild sphinx_rtd_theme breathe sphinx_tabs

Then you can access the documentation from ``http://127.0.0.1:8000`` with the following command. It is useful to use sphinx-autobuild plugin if you want to get immediate visual update on editing. Note that the documentation extracted from C++ sources are not updated automatically. Make sure to execute ``doxygen`` command again if you want to update the information.

.. code-block:: bash

   $ cd doc && doxygen
   $ cd ..
   $ sphinx-autobuild --watch src doc doc/_build/html

.. ----------------------------------------------------------------------------

.. Working with Jupyter notebook
.. =============

.. Install dependencies

.. .. code-block:: bash

..    $ conda install -c conda-forge jupyter matplotlib imageio
..    $ pip install tqdm 

.. ----------------------------------------------------------------------------

Running tests and examples
==========================

Running tests
-------------

To execute unit tests of the framework, run the following command after build.

.. code-block:: bash

   $ cd <lightmetrica binary dir>
   $ ./lm_test

Additionally, you can execute the Python tests with the following commands.

.. code-block:: bash

   $ conda install -c conda-forge pytest
   $ cd <root directory of lightmetrica>
   $ python -m pytest --lm <lightmetrica binary dir> pytest

Running examples
----------------

To execute all examples at once, run 

.. code-block:: bash

   $ cd example
   $ python run_all.py --lm <lightmetrica binary dir> --scene <scene dir>
   $ python compress_images.py --dir .
