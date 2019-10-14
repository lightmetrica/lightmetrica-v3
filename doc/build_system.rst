Build System
########################

This section descripts design choices about the build system of Lightmetrica Version 3.

Modern CMake
==================

The build system of Lighmetrica is organized by CMake scripts. In Lightmetrica Version 3, we refactored the scripts with modern design patterns. The updates include: target-based configuration, handling of the transitive dependencies (``PUBLIC``/``PRIVATE`` targets), ``INTERFACE`` target for header-only dependencies, exported targets along with installation, and a preference to the Config mode package finding. 

Dependency management
====================================

CMake can find external package with find_package command. This command finds the package based on the two different modes: Module mode and Config mode. To explain the difference, let's assume we have an application ``A`` and its dependent library ``B``. In Module mode, the application ``A`` finds the library ``B`` using ``Find*.cmake`` file (find-module). CMake distribution contains ``Find*.cmake`` files for famous libraries (e.g., boost) yet in most cases the developer of the application needs to provide the file. On the other hand, in Config mode, the application ``A`` finds the library ``B`` based on ``*Config.cmake`` file distributed along with the library. To support Config mode, the library needs to generate and install ``*Config.cmake`` file in the specific relative location in the installation directory. In other words, the difference between two modes is that in Module mode the application is responsible for finding the library, on the other hand in Config mode, the library is responsible for being found by the application. 

In the previous versions of the framework, we used Module mode packages to find dependent libraries. We found, however, that using Module mode complicates the build process if the library is used as a library of other application because the application also needs to resolve the transitive dependencies of the library. For instance, this can happen in our case, e.g., when an experiment written in C++ (e.g., interactive visualization) wants to reference Lightmetrica as a dependent library. This means the library needs to distribute find-modules of the transitive dependencies as well as the module for the library itself. As a result, the developer of the application always needs to prepare for find-modules of the all dependencies including transitive dependencies.

In this version, on the contrary, we decided to use Config mode packages which can faithfully handle the transitive dependencies. Also, since Config mode packages are distributed along with the library distribution, the user does not need to prepare for the find-modules. To use Config mode packages throughout the build chain, we choose the dependencies which supports to export ``*Config.cmake`` files. If not supported, we patched the libraries.

Supporting add_submodule
====================================

On a small modification, Lightmetrica Version 3 can inject the library into the user application using either find_package command or ``add_submodule`` command. The latter approach is especially useful when you want to develop both experimental code and the framework managed in the different repository in the same time.


Dependency management
====================================

In the previous versions, all library dependencies must be installed by a developer using pre-built binaries or platform-dependent package manager or libraries built from the sources. As a result, the installation of the dependencies differs in accordance with the platform. For instance, we provided a pre-built binaries for Windows environment but in Linux environment the developer needs to build the libraries from the source.

In this version, on the other hand, the dependencies can be installed via `conda package manager <https://docs.conda.io/en/latest/>`_ irrespective to the platform. Conda is a package manager and environment management system being mainly used in the Python community. Although conda is mainly used to distribute Python packages, it is in fact a language-agnostic package manager. For instance, we can introduce C++ libraries such as boost with conda. Combined with config-mode packages of CMake, the installed libraries can be easily integrated into the application configured with CMake. In this version, we could successfully export the all the necessary library dependencies to conda. This means the build-time dependencies can only be installed with one conda command. The actual dependencies are listed in `environment.yml <https://github.com/lightmetrica/lightmetrica-v3/blob/master/environment.yml>`_.

Also, conda can manage its own separated environments where you can manage different sets of the conda packages. It is useful when we want to combine different versions of the library in the same machine. For instance, two different versions of the framework might depend on the different versions of the library. In this case, it is not feasible to maintain both versions in the system-wide package manager, but separating the environment can manage two different versions simultaneously. It is especially important in research context since this situation often happens for instance when we need to maintain different experiments written for the different projects against the different versions of the framework.
