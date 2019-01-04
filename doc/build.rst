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
=============

First you want to setup the external dependencies.
The external dependencies of the framework is all header-only libraries.
We suppose two strategies to introduce dependencies based on whether to use 1) in-project directory or 2) pre-installed packages.

Using in-project directory
-------------

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
-------------

To use this strategy, you need to install the dependencies externally
using package management system or install the libraries from sources.
For instance, our Dockerfile adopts this strategy.
Please refer to the file for the detailed proceidure.

.. note::
   This strategy internally uses on CMake's ``find_package`` to find dependencies.
   Plus, Lightmetrica's CMake script always tries to find packages with `config mode`_.
   Unfortunately, some libraries did not expose well-defined configuration files. This may result to a failure in configuration step when we execute the cmake command.

   .. _config mode: https://cmake.org/cmake/help/latest/command/find_package.html#full-signature-and-config-mode

.. note::
   This strategy is mandatory when Lightmetrica is used as a library and integrated into the user's CMakefile with ``find_package`` command. This is because Lightmetrica's configuration script recursively calls ``find_package`` command to resolve transitive dependencies.
.. ----------------------------------------------------------------------------

Building the framework
=============

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



.. ----------------------------------------------------------------------------

Using Lightmetrica as external library
=============

We provide two different ways.

transitive dependencies.
note that lightmetrica has

.. ----------------------------------------------------------------------------

Editing documentation
=============

Install dependencies

.. code-block:: bash

   $ conda install -c conda-forge sphinx
   $ pip install sphinx-autobuild sphinx_rtd_theme breathe sphinx_tabs

Then you can access the documentation from ``http://127.0.0.1:8000`` with the following command. It is useful to use sphinx-autobuild plugin if you want to get immediate visual update on editing. Note that the documentation extracted from C++ sources are not updated automatically. Make sure to execute ``doxygen`` command again if you want to update the information.

.. code-block:: bash

   $ cd doc && doxygen
   $ cd ..
   $ sphinx-autobuild doc doc/_build/html

.. ----------------------------------------------------------------------------

.. Working with Jupyter notebook
.. =============

.. Install dependencies

.. .. code-block:: bash

..    $ conda install -c conda-forge jupyter matplotlib imageio
..    $ pip install tqdm 

.. ----------------------------------------------------------------------------

Running tests and examples
=============

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
   $ python -m pytest --lm <lightmetrica binary dir> lm/pytest

Running examples
-------------

To execute all examples at once, run 

.. code-block:: bash

   $ cd example
   $ python run_all.py --lm <lightmetrica binary dir> --scene <scene dir>
   $ python compress_images.py --dir .
