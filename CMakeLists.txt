#
#   Lightmetrica - Copyright (c) 2019 Hisanari Otsu
#   Distributed under MIT license. See LICENSE file for details.
#

cmake_minimum_required(VERSION 3.10)

# Enable policy to manage VERSION in project() command
cmake_policy(SET CMP0048 NEW)
project(lightmetrica VERSION 3.0.0)

# -------------------------------------------------------------------------------------------------

# Check if the directory is added via add_subdirectory
set(LM_MASTER_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(LM_MASTER_PROJECT ON)
endif()

# Build options
option(LM_USE_EXTERNAL_DIR   "Use in-project external directory to find dependencies" ON)
option(LM_INSTALL            "Enable install"  ${LM_MASTER_PROJECT})
option(LM_BUILD_TESTS        "Enable tests"    ${LM_MASTER_PROJECT})
option(LM_BUILD_EXAMPLES     "Enable examples" ${LM_MASTER_PROJECT})
option(LM_BUILD_GUI_EXAMPLES "Enable GUI examples" ${LM_MASTER_PROJECT})

# -------------------------------------------------------------------------------------------------

# CMake module path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# For CMAKE_INSTALL_*
include(GNUInstallDirs)

# PCH support
include(PrecompiledHeader)

# Use project filters in Visual Studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Enable the search of lib64 directory
set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS TRUE)

# -------------------------------------------------------------------------------------------------

# External dependencies
find_package(Threads REQUIRED)
find_package(OpenMP REQUIRED)
find_package(pybind11 REQUIRED)
find_package(cppzmq REQUIRED)
find_package(cereal REQUIRED)
find_package(glm REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(fmt REQUIRED)
find_package(stb REQUIRED)
if (LM_BUILD_TESTS)
    find_package(doctest REQUIRED)
endif()
if (LM_BUILD_EXAMPLES AND LM_BUILD_GUI_EXAMPLES)
    find_package(OpenGL)
    if (OpenGL_FOUND)
        find_package(GLEW REQUIRED)
        find_package(glfw3 REQUIRED)
    else()
        # Disable GUI example if there's no OpenGL support
        message(WARNING "OpenGL is not supported. Disabled GUI examples. \
                         To supress this warning, set LM_BUILD_GUI_EXAMPLES=OFF.")
        set(LM_BUILD_GUI_EXAMPLES OFF)
    endif()
endif()

# -------------------------------------------------------------------------------------------------

# Execute git command in the project root and check revision number
find_package(Git REQUIRED)
execute_process(
	COMMAND "${GIT_EXECUTABLE}" "rev-parse" "--short" "HEAD"
	OUTPUT_VARIABLE LM_VERSION_REVISION
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Current build timestamp
string(TIMESTAMP LM_BUILD_TIMESTAMP "%Y-%m-%d %H.%M.%S")

# Export framework version to a header
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/versiondef.h.in"
    "${PROJECT_BINARY_DIR}/versiondef.h"
    @ONLY
)
add_library(lm_version INTERFACE)
target_include_directories(lm_version INTERFACE "${PROJECT_BINARY_DIR}")

# -------------------------------------------------------------------------------------------------

# Main libraries
add_subdirectory(src)

# Plugins
add_subdirectory(plugin)

# Examples
if (LM_BUILD_EXAMPLES)
    #add_subdirectory(example)
endif()

# Tests
if (LM_BUILD_TESTS)
    add_subdirectory(test)
    add_subdirectory(pytest)
    add_subdirectory(functest)
endif()

# -------------------------------------------------------------------------------------------------

# Install
if (LM_INSTALL)
    # Install include directory
    install(DIRECTORY include/lm DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    
    # Package configuration directory
    set(LM_PACKAGE_CONFIG_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

    # ---------------------------------------------------------------------------------------------

    # Make package configuration files
    # Use configure_package_config_file to make the package relocatable
    include(CMakePackageConfigHelpers)
    # Generate *Config.cmake file
    configure_package_config_file(
        "${CMAKE_CURRENT_LIST_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        INSTALL_DESTINATION ${LM_PACKAGE_CONFIG_DIR}
    )
    # Generate *ConfigVersion.cmake file
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
    )
    # Install generated configuration files
    install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        DESTINATION ${LM_PACKAGE_CONFIG_DIR}
    )

    # ---------------------------------------------------------------------------------------------

    # Export targets
    # Make export target group
    install(
        EXPORT ${PROJECT_NAME}Targets
        FILE ${PROJECT_NAME}Targets.cmake
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION ${LM_PACKAGE_CONFIG_DIR}
    )
    # Generate *Target.cmake file
    export(
        EXPORT ${PROJECT_NAME}Targets
        FILE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake
        NAMESPACE ${PROJECT_NAME}::
    )
endif()
