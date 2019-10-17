Managing experiments
######################

Lightmetrica is designed to capable of conducting various experiments in rendering.
In this section, we will introduce the patterns for typical experimental setups using Lightmetrica.

We will suggest several use-cases of the typical experimental setups. Please jump to the corresponding sections according to your requirements.


Common setup
======================

In either cases, you want to setup conda environment to install the dependencies necessary to build Lightmetrica from source. We recommend to create multiple environment if you have multiple experiments
that depends on different versions (thus different dependencies) of the framework. Multiple environments can easily co-exist by assigning the environments with the different names.

.. code-block:: console

    $ cd <source directory>
    $ conda env create -n <name of the environment> -f environment.yml

.. note::

    If you omit ``-n`` option, the default name ``lm3_dev`` is used.

Creating environment includes downloading all dependencies and should take some time.
Once it is finished, you can activate the environment with

.. code-block:: console

    $ conda activate <name of the environment>

.. note::

    When you try to activate the environment at the first time, 
    depending on the conda version and your environment, you might be asked to call initial setup command ``conda init``.


Use-case: Basic experiments
============================================

This option is suitable if the requirements are:

1. You don't need to develop a plugin to conduct your experiments.
2. But you want to use pre-release feature that is not in the latest release yet.

Directory structure
--------------------------------

In this option, we suggest to use the following directory structure:

.. code-block:: plain

    - project_root/                 - (1)
        - your_experiment_1.ipynb   - (2)
        - your_experiment_2.ipynb
        - ...
        - lightmetrica-v3/          - (3)
            - build                 - (4)
            - build_docker          - (4-2)
            - ...
        - lmenv.py                  - (5)
        - .lmenv                    - (6)
        - .lmenv_docker             - (6-2)
        - .lmenv_debug              - (6-3)
        - ...
        - .gitignore                - (7)

The project root directory (1) manages all the experiments related codes (2) and cloned instance of Lightmetrica *inside* the project directory (3). We recommend this configuration because different projects might depends on the feature from the different versions of the framework.
The framework is build under the ``build`` directory (4). Refer to :ref:`building_from_source` for the instruction of how to build the framework.

Using ``lmenv`` module
--------------------------------

In order to utilize Lightmetrica from the experimental scripts, the scripts needs to be able to find appropriate Lightmetrica distribution in the Python's module search path (see also :ref:`working_with_jupyter_notebook`). To reduce a burden, we recommend to use ``lmenv`` module (5), which can be found in ``functest`` directory of the repository. You can copy and paste that module to your project directory.

The module provides ``lmenv.load()`` function that takes the path to the configuration file containing paths to the Lightmetria distribution:

.. code-block:: ipython

    In [1]: import lmenv
       ...: env = lmenv.load('.lmenv')

Here, ``.lmenv`` is a JSON file containing a object whose elements are specifying paths to the Lightmetrica distribution and binaries. It reads for instance:

.. code-block:: JSON

    {
        "path": "c:/path/to/project_root/lightmetrica-v3",
        "bin_path": "c:/path/to/project_root/lightmetrica-v3/build/bin/Release",
        "scene_path": "c:/path/to/scene_directory"
    }

``.lmenv`` file must contain at least two elements: (a) ``path`` specifying the path to the root directory of Lightmetrica, (b) ``bin_path`` specifying the path to the binary directory of Lightmetrica. Aside from them, the file can include any information that might be used globally among the experiments (e.g., path to the scene directory). The loaded elements can be accesed via namespace under ``env``:


.. code-block:: ipython

    In [2]: env.scene_path
    Out[2]: 'c:/path/to/scene_directory'

Managing multiple profile
--------------------------------

If you prepare multiple ``.lmenv`` files you can configure multiple profiles in the same directory. This is useful for instance when you want to conduct the same experiment in docker environment (6-2), or create profiles for different build configurations such as Release, Debug, etc. (6-3)

Managing directory as a git repository
-----------------------------------------

You can manage a project directory as a git repository. To do this, you want to configure appropriate ``.gitignore`` file (7) excluding ``lightmetrica-v3`` directory and machine-specific files like ``.lmenv``, since it may include machine-specific fullpaths. Alternatively, you can add lightmetrica-v3 as a submodule. 

lso, `jupytext <https://github.com/mwouts/jupytext>`_ Jupyter notebook extension is useful to maange Jupyter notebooks inside a git repository. The extension is already installed if you have the environment via ``environment.yml``.

Multi-platform development
--------------------------------

Assume we are using Windows environment (with Msys's bash) and also want to conduct the experiment in Linux environment using docker with the same revision of the code cloned into ``lightmetrica-v3`` directory (3). For a docker image, we use ``Dockerfile.conda`` distributed along with the framework. We assume we created ``lm3_dev`` image following the instruction in :ref:`dockerfile_only_with_dependencies`.

The following command executes a new container with an interactive session:

.. code-block:: console

    $ winpty docker run --rm -p 10000:8888 \
        -v <local projects directory>:/projects -it lm3_dev

We used ``-v`` option to share the local project directory containing ``project_root`` (1). We recommend to share parent directory as well as the project directory, because we might want to share also the shared resources like scenes. We used ``-p`` option to expose the port 8888 as a local port 10000 to use Jupyter notebook running inside the container.

.. note::

    Please be careful that the full path must start with ``//c/`` instead of ``c:/`` and 
    we must use ``winpty`` to use interactive session in Msys's bash.

Then we can build the framework being accessed through the shared volume. For detail, see :ref:`building_from_source`. 

.. code-block:: console

    # cd /projects/project_root/lightmetrica-v3
    # mkdir build_docker
    # ...

``lm3_dev`` image already installed dependencies to execute Jupyter notebook inside the docker container. You can execute the same experimental scripts from inside the docker container. Some options were necessary to prevent privilege errors or just for convenience.

.. code-block:: console

    # cd /projects/project_root
    # jupyter notebook --ip=0.0.0.0 --allow-root --NotebookApp.token=''

Here you can use ``.lmenv_docker`` file (6-2) to configure the path to the binaries. Note that you must specify absolute paths inside the container.

.. code-block:: JSON

    {
        "path": "/projects/project_root/lightmetrica-v3",
        "bin_path": "/projects/project_root/lightmetrica-v3",
        "scene_path": "/projects/..."
    }

.. Use-case: 
.. ============================================

.. Debugging experiments
.. --------------------------------

.. You can prepare multiple versions of `.lmenv` file 