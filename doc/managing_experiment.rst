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

.. _use_case_for_framework_users:

Use-case for framework users
============================================

This option is suitable for the *user* of the framework.
You want to choose this option if your requirements are

- not to develop a plugin to conduct your experiments,
- and to use pre-release feature that only accessible from the latest source.

Directory structure
--------------------------------

In this option, we suggest to use the following directory structure:

.. code-block:: plain

    - project_root/                 # (1)
        - your_experiment_1.ipynb   # (2)
        - your_experiment_2.ipynb
        - ...
        - lightmetrica-v3/          # (3)
            - build/                # (4)
            - build_docker/         # (4-2)
            - ...
        - lmenv.py                  # (5)
        - .lmenv                    # (6)
        - .lmenv_docker             # (6-2)
        - .lmenv_debug              # (6-3)
        - ...
        - .gitignore                # (7)

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

Also, `jupytext <https://github.com/mwouts/jupytext>`_ Jupyter notebook extension is useful to maange Jupyter notebooks inside a git repository. The extension is already installed if you have the environment via ``environment.yml``.

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

Then you can access the notebooks from ``http://localhost:10000`` from the brower in the host.

For the experimental scripts, you can use ``.lmenv_docker`` file (6-2) to configure the path to the binaries. Note that you must specify absolute paths inside the container.

.. code-block:: JSON

    {
        "path": "/projects/project_root/lightmetrica-v3",
        "bin_path": "/projects/project_root/lightmetrica-v3",
        "scene_path": "/projects/..."
    }

Use-case for plugin developers
============================================

This options is suitable if the requirements are

- to develop your own plugin to conduct your experiments,
- but not to want to modify the code of the framework itself.

Note that this option shares many patterns in :ref:`use_case_for_framework_users`, which we will not repeat the explanation in this section.

Directory structure
--------------------------------

The recommended directory structure is almost same as what we saw in :ref:`use_case_for_framework_users`, yet we moved ``build`` directory (1) to just under the project root.

.. code-block:: plain

    - project_root/
        - build/                    # (1)
        - CMakeLists.txt            # (2)
        - your_plugin_1.cpp         # (3)
        - ...
        - your_experiment_1.ipynb   # Experimental scripts
        - your_experiment_2.ipynb
        - ...
        - lightmetrica-v3/          # Framework clone
        - lmenv.py                  # Helper module to find the framework
        - .lmenv                    # Machine-dependent configuration
        - .lmenv_debug              # (4)
        - ...
        - .gitignore

Managing build
--------------------------------

Although it is possible to directly add your codes to the framework directory under ``lightmetrica-v3``,
we recommend to separate your plugin codes (3) to the outside of the framework directory unless necessary. This simplifies the management of your plugin codes via separated repository, and avoids possible merging burdens due to the upcoming updates of the framework. 

We create ``CMakeLists.txt`` to manage the build of the framework and your plugins. In CMakeLists, Lightmetrica supports direct inclusion of the directory via ``add_subdirectory``. Your ``CMakeLists.txt`` would look like

.. code-block:: cmake

    cmake_minimum_required(VERSION 3.10)
    project(your_experimental_project)

    # Add Lightmetrica via add_subdirectory
    add_subdirectory(lightmetrica-v3)

    # Craete plugins
    add_library(my_plugin MODULE "your_plugin_1.cpp")
    target_link_libraries(my_plugin PRIVATE lightmetrica::liblm)
    set_target_properties(my_plugin PROPERTIES DEBUG_POSTFIX "-debug")
    set_target_properties(my_plugin PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    # ...

``my_plugin`` is the target for your plugin. The library type should be ``MODULE`` because a plugin is dynamically loaded from the framework. Linking the target against ``lightmetrica::liblm`` allows the target to access the features of Lightmetrica.

You can build this project similarly as we described in :ref:`building_from_source`. For instance, in Windows environment (with Msys's bash):

.. code-block:: console

    $ cd <project_root>
    $ mkdir build && cd build
    $ cmake -G "Visual Studio 15 2017 Win64" ..
    $ start your_experimental_project.sln

Note that solution name is not ``lightmetrica.sln`` but ``your_experimental_project.sln`` as you wrote in ``CMakeLists.txt``. You also want to configure ``.lmenv`` file to the appropriate path to the build directory.

In your experimental scripts, you can load your plugin via :cpp:func:`lm::comp::loadPlugin` function. We assume we already configured ``env`` with ``lmenv.load()`` function.

.. code-block:: ipython

    In [1]: lm.comp.loadPlugin(os.path.join(env.bin_path, 'my_plugin'))
    

.. note::

    Once you execute the ``loadPlugin`` function, the Python process locks the shared library from further modification, which causes a compilation error when you update the code. To prevent this, you want to restart the kernel before compilation (``Kernel > Restart`` from the menu, or shortcut ``0-0``). 

Debugging experiments
--------------------------------

In Windows
~~~~~~~~~~~~~~~~~~~~~~

You might want to debug your plugin to fix bugs using a debugger.
In this section, we will describe how to debug a plugin using Visual Studio debugger. 

To do this, you want to first build the framework and your plugin in ``Debug`` build configuration.
To manage the path to the debug binaries, we recommend to create another ``.lmenv_debug`` file containing path to the debug binary path (4):

.. code-block:: JSON
    :emphasize-lines: 3

    {
        "path": "c:/path/to/project_root/lightmetrica-v3",
        "bin_path": "c:/path/to/project_root/build/bin/Debug",
        "scene_path": "c:/path/to/scene_directory"
    }

You can easily switch two profiles by directly changing the string in the experimental script to

.. code-block:: ipython

    In [1]: import lmenv
       ...: env = lmenv.load('.lmenv_debug')

.. note::

    You must restart the kernel after you modify a reference to ``.lmenv`` file,
    since Python process holds a reference to the binary in the previously specified ``.lmenv`` file.


The experimental scripts are written in Python and executed in a Jupyter notebook. To debug the dynamically loaded plugin from the framework, you thus need to use ``Attach to Process`` feature of Visual Studio debugger (from menu, ``Debug > Attach to Process...``). However, this feature needs to locate the Python process you are currently focusing on. Typically you will find multiple Python processes in the list and cannot identify which is the process to which you want to attach by name. You can also use a process ID to locate the process, which can be obtained by calling ``os.getpid()`` function from inside the notebook.

However, you need to iterate this process once you restarted the kernel since the process ID changes every time. Also, since you need to restart the kernel every time you update the binary, you need to iterate this process every time you recompile the code. Using recently added feature of ``Reattach to Process`` is also not usable, because it detects the previously attached process by name (there's always multple choices) or process ID (changes every time).

To resolve the problem, we provide :cpp:func:`lm::debug::attachToDebugger` function to attach to the debugger from the Python code. Once the function is called, it opens a dialog to select an instance of Visual Studio (``vsjitdebugger.exe``). You want to select the opening Visual Studio instance and press Yes, then the debugger is launched with the Python process being attached to the debugger.

Typically, you want to call this function only if you are running the code with Debug or RelWithDebInfo build configuations, which can be checked by examing ``lm.BuildConfig``:

.. code-block:: ipython

    In [1]: if lm.BuildConfig != lm.ConfigType.Release:
       ...:     lm.debug.attachToDebugger()


.. note::

    :cpp:func:`lm::debug::attachToDebugger` uses Windows specific feature thus is only supported in Windows environment.