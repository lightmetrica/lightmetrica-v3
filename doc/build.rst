.. _build:

Build
############

This section describes build process of Lightmetrica.


Prerequisites
=============

We recommend to use miniconda_ to manage build and runtime environment of the framework.

.. _miniconda: https://docs.conda.io/en/latest/miniconda.html

.. ----------------------------------------------------------------------------

Installing framework
==========================

There are two different options to use the framework. You want to choose either of them according to your requirements.

1. **Using pre-built binaries**. This option is suitable when you want to use the framework as a renderer without user-defined extensions, or when you want to develope your own plugins but you don't need to extend the framework itself.

2. **Bulilding from source**. This option is suitable for for the framework developers, or for the situation where you have the requirements that cannot be achieved only by plugins. If you want to use the framework for the research purpose, we would recommend to use this option.

.. ----------------------------------------------------------------------------

Using pre-built binaries
==========================

Installing conda package
--------------------------

We distribute the pre-built binaries of the framework as a conda package.
You can easily install the framework including dependencies by conda install command.
This option is especially useful when you don't want to modify the framework itself.
To install the framework, you just need the following commands:

.. code-block:: console

    $ conda create -n lm3 python=3.7
    $ conda activate lm3
    $ conda install lightmetrica -c hi2p-perim -c conda-forge

The first line create a new conda environment with Python 3.7.
Currently, the pre-built package only supports Python 3.7 and other Python versions are not supported.
If you want to use different Python version, consider to build the framework by yourself.

If you could successfully install the framework, executing the following commands will get you the information of the framework. Note that python command must be executed in the same conda environment.

.. code-block:: console

    $ python
    >>> import lightmetrica as lm
    >>> lm.init()
    >>> lm.info()

.. note::

    ``-c hi2p-perim`` option must precedes `-c conda-forge` option, since ``conda install`` prioritizes the first channel specified when the packages with the same name are found in both channels.


Building plugins
--------------------------

We can also easily build Lightmetrica plugins using the pre-built binaries. To do this, you need CMake (>=3.10) and build environments according to the platform: Visual Studio 2017 or 2019 (Windows) and GCC 7 (Linux).

The package is shipped with development files for the plugin development. The distributed binaries are built with Visual Studio 2017 or 2019 (Windows) and GCC 7 (Linux).
Although you can develop plugins as long as the ABI compatibility allows,
we highly recommend to use the compiler with the same version in which the framework is built.
If you want to use compilers that might break ABI compatibility for instance due to the change of compiler versions, consider to build the framework by yourself.

.. note::
    As a development goes by, you might want to change the pre-built binaries to your own build.
    In this case, we highly recommends to create a fresh new conda environment
    to avoid serious dependency issues that might be caused by having both builds in the same environment.

To build a plugin, you want to make a ``CMakeLists.txt`` file like:

.. code-block:: cmake

    cmake_minimum_required(VERSION 3.10)
    project(test_plugin)
    find_package(lightmetrica REQUIRED)
    add_library(test_plugin MODULE "test_plugin.cpp")
    target_link_libraries(test_plugin PRIVATE lightmetrica::liblm)

We use ``find_package`` command to find ``lightmetrica`` package.
The command automatically finds the corresponding pre-build binary installed in the conda environment. ``target_link_libraries`` command specifies the target to which the plugin is linked. Here, we specify ``lightmetrica::liblm`` target. Then we can build the plugin with the following comamnds.

In Windows:

.. code-block:: console

    $ cd <plugin source directory>
    $ mkdir build
    $ cd build
    $ cmake -G "Visual Studio 15 2017 Win64" ..
    // or $ cmake -G "Visual Studio 16 2019" -T v141 ..
    $ start test_plugin.sln

In Linux:

.. code-block:: console

    $ cd <plugin source directory>
    $ mkdir build && cd build
    $ cmake -DCMAKE_BUILD_TYPE=Release ..
    $ make -j

.. note::
    It is important to execute the aforementioned commands from the activated conda environment.
    Otherwise CMake cannot find the ``lightmetrica`` package installed in the environment.
    If you are using Windows and you want to apply the configuration of the environment also in Visual Studio, use ``start`` command from the terminal to launch the IDE.

.. note::
    The option ``-DCMAKE_BUILD_TYPE=Release`` is necessary because
    CMake's default is ``Debug`` in Linux environment.


.. note::
    Visual Studio 2019 is supported by ``v141`` toolset. You need to install it from the VS installer. 
    The newer toolsets might not work due to the binary incompatibilities.

.. ----------------------------------------------------------------------------

.. _building_from_source:

Building from source
==========================

Installing dependencies
--------------------------

We distribute the external dependencies as conda packages.
We recommend to use a separated environment to manage the build environment.
The dependent packages are written in ``environment.yml``.
From the file, you can create and activate a new conda environment named ``lm3_dev`` with the following commands. All commands in the following instruction assume the activation of the ``lm3_dev`` environment.

.. code-block:: console

    $ conda env create -f environment.yml
    $ conda activate lm3_dev

If you want to create the environment with an original name, you can use ``-n`` option.

.. code-block:: console

    $ conda env create -n <preferred name> -f environment.yml
    $ conda activate <preferred name>

Building framework
--------------------------

Before building, you want to clone the repository to your local directory. Be careful to use ``--recursive`` option since the repository contains submodules.

.. code-block:: console

    $ git clone --recursive git@github.com:lightmetrica/lightmetrica-v3.git

.. note::

    If you already cloned the repository without ``--recursive`` option, you must initialize the submodules with ``git submodule init`` and ``git submodule update`` commands.


**Windows**. You can generate solution for Visual Studio with the following commands.
To build Python binding, be sure to activate the previously-created Python environment and start Visual Studio from the same shell.

.. code-block:: console

    $ cd <source directory>
    $ mkdir build && cd build
    $ cmake -G "Visual Studio 15 2017 Win64" ..
    $ start lightmetrica.sln


**Linux**. The following commands generates the binaries under ``build/bin`` directory.

.. code-block:: console

    $ mkdir build && cd build
    $ cmake -DCMAKE_BUILD_TYPE=Release ..
    $ make -j

Alternatively, execute the following command to build and install Lightmetrica to your system. If you want to change installation directory, add ``-DCMAKE_INSTALL_PREFIX=<install dir>`` to the previous ``cmake`` command.

.. code-block:: console

   $ cmake --build build --target install

Using source-built framework as external library
----------------------------------------------------

If you want to use the source-built version of Lightmetrica in your project,
you can directly include the source directory of Lightmetrica using ``add_subdirectory`` command in your ``CMakeLists.txt`` file. This approach is useful when you want to maintain your experiments outside of the framework repository while modifying the framework itself.

The following ``CMakeLists.txt`` shows minimum example of this approach. 
Once you include the directory, you can use ``lightmetrica::liblm`` target to link the main library ``lightmetrica::liblm`` to your application or plugin.

.. code-block:: cmake
    :emphasize-lines: 3

    cmake_minimum_required(VERSION 3.10)
    project(your_renderer)
    add_subdirectory(lightmetrica)
    add_executable(your_renderer "your_renderer.cpp")
    target_link_libraries(your_renderer PRIVATE lightmetrica::liblm)



.. ----------------------------------------------------------------------------

Editing documentation
==========================

It is useful to use sphinx-autobuild plugin if you want to get immediate visual update on editing. 
With the following commands, you can access the documentation from ``http://127.0.0.1:8000``. Note that the documentation extracted from C++ sources are not updated automatically. Make sure to execute ``doxygen`` command again if you want to update the information.

.. code-block:: console

    $ cd <source directory>
    $ cd doc && mkdir _build && doxygen
    $ cd -
    $ sphinx-autobuild --watch src doc doc/_build/html

Note that some documentation is generated from the executed Jupyter notebooks.
To obtain a complete documentation, you want to execute the notebooks in ``functest`` directory
and copy the executed notebooks to the ``doc/executed_functest`` directory.
For detail, please find ``.travis.yml`` file.

.. ----------------------------------------------------------------------------

.. _working_with_jupyter_notebook:

Working with Jupyter notebook
=============================

Move to your working directory, and execute Jupyter notebook

.. code-block:: console

    $ cd <working directory>
    $ jupyter-notebook

Example of starting cells, where [1] loads ``lightmetrica_jupyter`` extension
and [2] copies Release binaries to temporary directory
and [3] imports the framework as an alias ``lm``:

.. code-block:: ipython

    In [1]: import sys
       ...: sys.path.append(r'<Lightmetrica root directory>')
       ...: sys.path.append(r'<Lightmetrica binary directory>')
    In [2]: %load_ext lightmetrica_jupyter
    In [3]: import lightmetrica as lm

We provide Jupyter notebook friendly implementation of :cpp:class:`lm::Logger` and :cpp:class:`lm::Progress`.
To use the recommended settings, use ``jupyter_init_config()`` function and append the return value
to the argument of :cpp:func:`lm::init()` function.

.. code-block:: ipython

    In [4]: from lightmetrica_jupyter import jupyter_init_config
    In [5]: lm.init('user::default', {<other configuration>, **jupyter_init_config()})

.. note::

   IPython kernel locks the loaded c extensions
   and prevents the shared libraries of the framework from being recompiled,
   until the kernel is shut down.
   Thus if you want to rebuild already-loaded c extension you need to first shutdown the kernel.

.. ----------------------------------------------------------------------------

Working with Docker containers
==============================

We prepared Dockerfiles to setup linux environments for several use-cases.

Dockerfile for build and tests
-------------------------------------

``Dockerfile`` in the root directory of the framework setups the dependencies with conda packages and builds the framework, followed by the execution of the unit tests. The Dockerfile is also used in the automatic build with CI service. The following commands build a docker image ``lm3`` and run an interactive session with the container.

.. code-block:: console

    $ docker build -t lm3 .
    $ docker run --rm -it lm3

.. note::

    For Windows users: running interactive session with docker in Msys's bash (incl. Git bash) needs ``winpty`` command before the above ``docker run`` command. Also, if you want to specify the shared volume with ``-v`` option, you need to use the path starting from ``//c/`` instead of ``c:/``.

.. _dockerfile_only_with_dependencies:

Dockerfile only with dependencies
-------------------------------------

``Dockerfile.conda`` configures conda dependencies of Lightmetrica as an docker image. Unlike ``Dockerfile``, this image does not build the framework by default. This docker image is useful when you want to share the code with host machine while development.

.. code-block:: console

    $ docker build -t lm3_dev -f Dockerfile.conda .
    $ docker run --rm -it lm3_dev

