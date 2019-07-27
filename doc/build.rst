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

2. **Bulilding from source**. This option is suitable for for the framework developers, or for the situation where you have the requirements that cannot be achieved only by plugins.

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
    $ conda install lightmetrica -c conda-forge -c hi2p-perim

The first line create a new conda environment with Python 3.7.
Currently, the pre-built package only supports Python 3.7 and other Python versions are not supported.
If you want to use different Python version, consider to build the framework by yourself.

If you could successfully install the framework, executing the following commands will get you the information of the framework. Note that python command must be executed in the same conda environment.

.. code-block:: console

    $ python
    >>> import lightmetrica as lm
    >>> lm.init()
    >>> lm.info()

Building plugins
--------------------------

We can also easily build Lightmetrica plugins using the pre-built binaries. To do this, you need CMake (>=3.10) and build environments according to the platform: Visual Studio 2017 (Windows) and GCC 7 (Linux).

The package is shipped with development files for the plugin development. The distributed binaries are built with Visual Studio 2017 (Windows) and GCC 7 (Linux).
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

.. ----------------------------------------------------------------------------

.. _building_from_source:

Building from source
==========================

Installing dependencies
--------------------------

We distribute the external dependencies as conda packages.
We recommend to use a separated environment to manage the build environment.
The dependent packages are written in ``environment_win.yml`` (Windows) or ``environment_linux.yml`` (Linux).
From the file, you can create and activate a new conda environment named ``lm3_dev`` with the following commands. All commands in the following instruction assume the activation of the ``lm3_dev`` environment.

.. code-block:: console

    $ conda env create -f environment_{win,linux}.yml
    $ conda activate lm3_dev

Building framework
--------------------------

Before building, you want to clone or download the repository to your local directory.

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

``Dockerfile`` in the root directory of the framework setups the dependencies with conda packages and builds the framework,
followed by the execution of the unit tests. The Dockerfile is also used in the automatic build with CI service.
The following commands build a docker image ``lm3``.

.. code-block:: console

    $ docker build -t lm3 .

``Dockerfile.jupyter`` is made for the development with Jupyter notebook
where the source directory of Lightmetrica is supposed to be mounted from the host. 
Our Dockerfile is based on Jupyter's `docker-stacks`_.
The following commands create an image ``lm3_jupyter`` and execute a notebook server as a container.
For convenience, we often mount workspace and scene directories in addition to the source directory.

.. _`docker-stacks`: https://github.com/jupyter/docker-stacks

.. code-block:: console

    $ docker build -t lm3_jupyter -f ./Dockerfile.jupyter .
    $ docker run \
        --cap-add=SYS_PTRACE --security-opt seccomp=unconfined \
        -it --rm -p 8888:8888 -h lm3_docker \
        -v ${PWD}:/lightmetrica-v3 \
        -v <workspace directory on host>:/work \
        -v <scene directory on host>:/scenes \
        lm3_jupyter start-notebook.sh \
            --NotebookApp.token='<access token for notebook>' \
            --ip=0.0.0.0 --no-browser

``Dockerfile.desktop`` is made for the development with Linux desktop environment, specifically from Windows host.
We used `docker-ubuntu-vnc-desktop`_ to setup LXDE desktop environment on Ubuntu, which utilizes `noVNC`_ for browser-based VNC connection.
After executing the commands, you can access the desktop via ``localhost:6080`` using a browser.

.. _`docker-ubuntu-vnc-desktop`: https://github.com/fcwu/docker-ubuntu-vnc-desktop
.. _`noVNC`: https://novnc.com

.. code-block:: console

    $ docker build -t lm3_desktop -f ./Dockerfile.desktop .
    $ docker run \
        --cap-add=SYS_PTRACE --security-opt seccomp=unconfined \
        --rm -p 6080:80 -p 5900:5900 -e RESOLUTION=1920x1080 \
        -v ${PWD}:/lightmetrica-v3 \
        -v <workspace directory on host>:/work \
        -v <scene directory on host>:/scenes \
        lm3_desktop

.. note::

   The arguments ``--cap-add=SYS_PTRACE --security-opt seccomp=unconfined`` are necessary
   to execute the applications with gdb in docker containers.