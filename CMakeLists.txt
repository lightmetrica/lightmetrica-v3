#
#   Lightmetrica - Copyright (c) 2019 Hisanari Otsu
#   Distributed under MIT license. See LICENSE file for details.
#

cmake_minimum_required(VERSION 3.10)

# Enable policy to manage VERSION in project() command
cmake_policy(SET CMP0048 NEW)
project(lightmetrica VERSION 3.0.0 LANGUAGES CXX)

# -----------------------------------------------------------------------------

# Check if the directory is added via add_subdirectory
set(LM_MASTER_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(LM_MASTER_PROJECT ON)
endif()

# Build options
option(LM_USE_EXTERNAL_DIR "Use in-project external directory to find dependencies" ON)
option(LM_INSTALL          "Enable install"  ${LM_MASTER_PROJECT})
option(LM_BUILD_TESTS      "Enable tests"    ${LM_MASTER_PROJECT})
option(LM_BUILD_EXAMPLES   "Enable examples" ${LM_MASTER_PROJECT})

# -----------------------------------------------------------------------------

# CMake module path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# For CMAKE_INSTALL_*
include(GNUInstallDirs)

# PCH support
include(PrecompiledHeader)

# Use project filters in Visual Studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# -----------------------------------------------------------------------------

# OpenMP
find_package(OpenMP REQUIRED)

# Directory for in-project external dependencies
set(LM_EXTERNAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external")

# Include dependencies via add_subdirectory
# when the correponding in-project external directory exists.
# Otherwise use find_package via config mode.

# nlohmann-json
# https://github.com/nlohmann/json
if (LM_USE_EXTERNAL_DIR AND EXISTS "${LM_EXTERNAL_DIR}/json")
    option(JSON_BuildTests "" OFF)
    option(JSON_MultipleHeaders "" OFF)
    add_subdirectory(external/json)
else()
    find_package(nlohmann_json REQUIRED)
endif()

# OpenGL mathematics (glm)
# https://github.com/g-truc/glm
if (LM_USE_EXTERNAL_DIR AND EXISTS "${LM_EXTERNAL_DIR}/glm")
    option(GLM_TEST_ENABLE "" OFF)
    add_subdirectory(external/glm)
else()
    find_package(glm REQUIRED)
endif()

# doctest
# https://github.com/onqtam/doctest
if (LM_USE_EXTERNAL_DIR AND EXISTS "${LM_EXTERNAL_DIR}/doctest")
    option(DOCTEST_WITH_TESTS "" OFF)
    option(DOCTEST_NO_INSTALL "" ON)
    add_subdirectory(external/doctest)
    # We need this line because exported target uses doctest::doctest
    # while add_subdirectory only finds doctest target.
    add_library(doctest::doctest ALIAS doctest)
else()
    find_package(doctest REQUIRED)
endif()

# pybind11
# https://github.com/pybind/pybind11
if (LM_USE_EXTERNAL_DIR AND EXISTS "${LM_EXTERNAL_DIR}/pybind11")
    add_subdirectory(external/pybind11)
else()
    find_package(pybind11 REQUIRED)
endif()

# fmt
# https://github.com/fmtlib/fmt
if (LM_USE_EXTERNAL_DIR AND EXISTS "${LM_EXTERNAL_DIR}/fmt")
    add_subdirectory(external/fmt)
else()
    find_package(fmt REQUIRED)
endif()

# cereal
# https://github.com/USCiLab/cereal
if (LM_USE_EXTERNAL_DIR AND EXISTS "${LM_EXTERNAL_DIR}/cereal")
    option(JUST_INSTALL_CEREAL "" ON)
    add_subdirectory(external/cereal)
else()
    find_package(cereal REQUIRED)
endif()

# -----------------------------------------------------------------------------

# PCH related
set(_PCH_DIR "${CMAKE_CURRENT_SOURCE_DIR}/pch")
set(_PCH_FILES
    "${_PCH_DIR}/pch.h"
    "${_PCH_DIR}/pch.cpp")

# Main libraries
add_subdirectory(src)

# Examples
if (LM_BUILD_EXAMPLES)
    add_subdirectory(example)
endif()

# Tests
if (LM_BUILD_TESTS)
    add_subdirectory(test)
    add_subdirectory(pytest)
endif()

# -----------------------------------------------------------------------------

# Install
if (LM_INSTALL)
    # Install include directory
    install(DIRECTORY include/lm DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    
    # Package configuration directory
    set(LM_PACKAGE_CONFIG_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

    # -------------------------------------------------------------------------

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

    # -------------------------------------------------------------------------

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
        NAMESPACE ${PROJECT_NAME}::)
endif()
