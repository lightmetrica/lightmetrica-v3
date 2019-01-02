Build
############

This section describes build process of Lightmetrica.


Windows
==================

Tested with Visual Studio 2017 Version 15.9

Prerequisites
-------------

- miniconda_

  - Recommended when you want to use Python binding to manage isolated environment for the dependencies requires for Lightmetrica.

- doxygen_

  - Optional when you want to generate documentation of Lightmetrica.

.. _miniconda: https://conda.io/miniconda.html
.. _doxygen: http://www.doxygen.nl/


Creating Python environment
-------------

The following commands create a new Python environment and install dependencies for Lightmetrica. Some dependencies are optional depending on your usage of the framework.

.. code-block:: bash

   $ conda create -n lm3 python=3.6
   $ source activate lm3
   $ conda install -c conda-forge sphinx jupyter matplotlib imageio
   $ pip install sphinx-autobuild sphinx_rtd_theme breathe sphinx_tabs pytest tqdm 

Editing documentation
-------------

You can access the documentation from ``http://127.0.0.1:8000`` with the following command. It is useful to use sphinx-autobuild plugin if you want to get immediate visual update on editing. Note that the documentation extracted from C++ sources are not updated automatically. Make sure to execute ``doxygen`` command again if you want to update the information.

.. code-block:: bash

   $ cd doc && doxygen
   $ cd ..
   $ sphinx-autobuild doc doc/_build/html

Building the framework
-------------

First you want to setup the external libraries.
All dependencies are placed in ``external`` directory.
Lightmetrica utilizes only the header-only libraries for the required dependencies.

.. code-block:: bash

   $ cd external
   $ git clone --depth 1 git@github.com:pybind/pybind11.git
   $ git clone --depth 1 git@github.com:nlohmann/json.git
   $ git clone --depth 1 git@github.com:g-truc/glm.git
   $ git clone --depth 1 git@github.com:onqtam/doctest.git
   $ git clone --depth 1 git@github.com:fmtlib/fmt.git
   $ git clone --depth 1 git@github.com:USCiLab/cereal.git
   $ git clone --depth 1 git@github.com:agauniyal/rang.git

Then you can generate solution for Visual Studio with the following commands. To build Python binding, be sure to activate the previously-created Python environment and start Visual Studio from the same shell.

.. code-block:: bash

   $ source activate lm3
   $ mkdir build && cd build
   $ cmake -G "Visual Studio 15 2017 Win64" ..
   $ start lightmetrica.sln

Running tests
-------------

To execute unit tests of the framework, run the following command after build.

.. code-block:: bash

   $ cd <lightmetrica binary dir>
   $ ./lm_test

Additionally, you can execute the Python tests with the following commands.

.. code-block:: bash

   $ cd <root directory of lightmetrica>
   $ python -m pytest --lm <lightmetrica binary dir> lm/pytest

Running examples
-------------

To execute all examples at once, run 

.. code-block:: bash

   $ cd example
   $ python run_all.py --lm <lightmetrica binary dir> --scene <scene dir>
   $ python compress_images.py --dir .
