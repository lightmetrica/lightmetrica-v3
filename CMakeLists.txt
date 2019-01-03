#
#   Lightmetrica - Copyright (c) 2019 Hisanari Otsu
#   Distributed under MIT license. See LICENSE file for details.
#

cmake_minimum_required(VERSION 3.12)
project(lightmetrica)

# -----------------------------------------------------------------------------

# CMake module path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# PCH support
include(PrecompiledHeader)

# Use project filters in Visual Studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# -----------------------------------------------------------------------------

# OpenMP
find_package(OpenMP REQUIRED)

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external")
    # nlohmann-json
    option(JSON_BuildTests "" OFF)
    option(JSON_MultipleHeaders "" OFF)
    add_subdirectory(external/json)
    # glm
    option(GLM_TEST_ENABLE "" OFF)
    add_subdirectory(external/glm)
    # doctest
    option(DOCTEST_WITH_TESTS "" OFF)
    option(DOCTEST_NO_INSTALL "" ON)
    add_subdirectory(external/doctest)
    # pybind11
    add_subdirectory(external/pybind11)
    # fmt
    add_subdirectory(external/fmt)
    # cereal
    option(JUST_INSTALL_CEREAL "" ON)
    add_subdirectory(external/cereal)
elseif()
    find_package(json REQUIRED)
    find_package(glm REQUIRED)
    find_package(doctest REQUIRED)
    find_package(pybind11 REQUIRED)
    find_package(fmt REQUIRED)
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
add_subdirectory(example)

# Tests
add_subdirectory(test)
add_subdirectory(pytest)
