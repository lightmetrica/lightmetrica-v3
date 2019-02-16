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
For convenience, we prepared an repository containing all external dependencies as submodules.
The following commands will setup all depenencies inside ``external`` directory.

.. code-block:: console

   $ git clone --recursive https://github.com/hi2p-perim/lightmetrica-v3-external.git external

.. note::
   This strategy internally uses on CMake's ``add_subdirectory`` to find dependencies.
   The options for the dependencies are configured to minimize the build, e.g., disabling builds of examples or tests.

.. _using_preinstalled_packages:

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

.. code-block:: console

   $ mkdir build && cd build
   $ cmake -G "Visual Studio 15 2017 Win64" ..
   $ start lightmetrica.sln


Linux
-------------

Tested with GCC 8.3 and `Ninja`_. The following commands generates the binaries under ``build/bin`` directory.

.. _Ninja: https://ninja-build.org/

.. code-block:: console

   $ mkdir build && cd build
   $ cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release ..
   $ ninja

.. note::
    
    The option ``-DCMAKE_BUILD_TYPE=Release`` is necessary because
    CMake's default is ``Debug`` in Linux environment.
   

Additionally, execute the following command to install Lightmetrica to your system. If you want to change installation directory, add ``-DCMAKE_INSTALL_PREFIX=<install dir>`` to the ``cmake`` command.

.. code-block:: console

   $ ninja install

.. ----------------------------------------------------------------------------

Using Lightmetrica as external library
=======================================

To use Lightmetrica as external library, you need to 
configure Lightmetrica as a dependency inside your own ``CMakeLists.txt``.
We again have two approaches, whether to use ``add_subdirectory`` or ``find_package``.

Using add_subdirectory
--------------------------

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
--------------------------

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

.. code-block:: console

   $ conda install -c conda-forge sphinx
   $ pip install sphinx-autobuild sphinx_rtd_theme breathe sphinx_tabs

Then you can access the documentation from ``http://127.0.0.1:8000`` with the following command. It is useful to use sphinx-autobuild plugin if you want to get immediate visual update on editing. Note that the documentation extracted from C++ sources are not updated automatically. Make sure to execute ``doxygen`` command again if you want to update the information.

.. code-block:: console

   $ cd doc && doxygen
   $ cd ..
   $ sphinx-autobuild --watch src doc doc/_build/html

.. ----------------------------------------------------------------------------

Running tests and examples
==========================

Running tests
-------------

To execute unit tests of the framework, run the following command after build.

.. code-block:: console

   $ cd <lightmetrica binary directory>
   $ ./lm_test

Additionally, you can execute the Python tests with the following commands.

.. code-block:: console

   $ conda install -c conda-forge pytest
   $ cd <root directory of lightmetrica>
   $ python -m pytest --lm <lightmetrica binary dir> pytest

Running examples
----------------

To execute all examples at once, run 

.. code-block:: console

   $ cd example
   $ python run_all.py --lm <lightmetrica binary dir> --scene <scene dir>
   $ python compress_images.py --dir .

.. ----------------------------------------------------------------------------

Working with Jupyter notebook
=============================

Install dependencies

.. code-block:: console

   $ conda install -c conda-forge jupyter matplotlib imageio
   $ pip install tqdm 

Move to your working directory, and create ``.lmenv`` file
where we describe the paths to the binary and scene directories of the framework.
Example of ``.lmenv`` file:

.. code-block:: json

    {
        "<hostname>": {
            "module_dir": {
                "Release": "<Path to release binary directory>",
                "Debug": "<Path to debug binary directory>"
            },
            "scene_dir": "<Scene path>"
        }
    }

Execute Jupyter notebook

.. code-block:: console

   $ cd <working directory>
   $ jupyter-notebook

Example of starting cells, where [1] loads ``lightmetrica_jupyter`` extension
and [2] copies Release binaries to temporary directory
and [3] imports the framework as an alias ``lm``:

.. code-block:: ipython

  In [1]: import sys
     ...: sys.path.append(r'<Lightmetrica root directory>')
  In [2]: %load_ext lightmetrica_jupyter
     ...: %update_lm_modules Release
  In [3]: import lightmetrica as lm

.. note::

   IPython kernel locks the loaded c extensions
   and prevents the shared libraries of the framework from being recompiled,
   until the kernel is shut down.
   To improve the efficiency of the workflow,
   we provide ``%update_lm_modules <configuration>`` line magic function.
   The function takes configuration ``Release`` or ``Debug`` as an argument,
   then copies the binaries to the temporary directory according to the configuration written in ``.lmenv``.

We provide Jupyter notebook friendly implementation of :cpp:class:`lm::Logger` and :cpp:class:`lm::Progress`.
To use the recommended settings, use ``jupyter_init_config()`` function and append the return value
to the argument of :cpp:func:`lm::init()` function.

.. code-block:: ipython

   In [4]: from lightmetrica_jupyter import jupyter_init_config
   In [5]: lm.init('user::default', {<other configuration>, **jupyter_init_config()})

.. ----------------------------------------------------------------------------

Working with Docker containers
==============================

We prepared Dockerfiles to setup linux environments for several use-cases.

``Dockerfile`` in the root directory of the framework setups the dependencies using the strategy described in :ref:`using_preinstalled_packages`,
and builds the framework, followed by the execution of the unit tests. The Dockerfile is also used in the automatic build with CI service.
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