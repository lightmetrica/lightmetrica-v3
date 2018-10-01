#
#   Lightmetrica - Copyright (c) 2018 Hisanari Otsu
#   Distributed under MIT license. See LICENSE file for details.
#

cmake_minimum_required(VERSION 3.10)
project(lm)

# -----------------------------------------------------------------------------

# nlohmann-json
option(JSON_BuildTests "" OFF)
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

# spdlog
add_subdirectory(external/spdlog)

# fmt
add_subdirectory(external/fmt)

# -----------------------------------------------------------------------------

# Main library
set(_PROJECT_NAME liblm)
set(_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/lm")
set(_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
add_library(${_PROJECT_NAME} SHARED
	# Headers
    "${_INCLUDE_DIR}/lm.h"
    "${_INCLUDE_DIR}/api.h"
    "${_INCLUDE_DIR}/common.h"
	"${_INCLUDE_DIR}/component.h"
	"${_INCLUDE_DIR}/logger.h"

	# Sources
    "${_SOURCE_DIR}/component.cpp"
    "${_SOURCE_DIR}/logger.cpp"
    "${_SOURCE_DIR}/api.cpp"
)

target_link_libraries(${_PROJECT_NAME}
    PUBLIC nlohmann_json glm fmt-header-only
    PRIVATE spdlog)
target_include_directories(${_PROJECT_NAME}
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")
target_compile_definitions(${_PROJECT_NAME} PRIVATE -DLM_EXPORTS)
target_compile_features(${_PROJECT_NAME} PUBLIC cxx_std_17)

# -----------------------------------------------------------------------------

# Python binding
set(_PROJECT_NAME pylm)
add_library(${_PROJECT_NAME} MODULE "src/pylm.cpp")
target_link_libraries(${_PROJECT_NAME} PRIVATE liblm pybind11::module)
set_target_properties(${_PROJECT_NAME} PROPERTIES
    PREFIX "${PYTHON_MODULE_PREFIX}"
    SUFFIX "${PYTHON_MODULE_EXTENSION}")

# -----------------------------------------------------------------------------

# Add a plugin
function(lm_add_plugin)
    cmake_parse_arguments(_ARG "" "NAME;INCLUDE_DIR" "SOURCE" ${ARGN})

    # INTERFACE library for headers
    add_library(${_ARG_NAME}_interface INTERFACE)
    add_library(${_ARG_NAME}::interface ALIAS ${_ARG_NAME}_interface)
    target_include_directories(${_ARG_NAME}_interface
        INTERFACE "$<BUILD_INTERFACE:${_ARG_INCLUDE_DIR}>")

    # MODULE library for the dynamic loaded library
    add_library(${_ARG_NAME} MODULE ${_ARG_SOURCE})
    target_link_libraries(${_ARG_NAME}
        PRIVATE liblm ${_ARG_NAME}_interface)
endfunction()

# -----------------------------------------------------------------------------

# Plugin for unit test
lm_add_plugin(
    NAME lm_test_plugin
    INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    SOURCE
        "test/test_interface.h"
        "test/test_interface_impl.cpp")

# Get python root directory
find_package(Python REQUIRED COMPONENTS Development)
get_filename_component(_PYTHON_ROOT ${Python_INCLUDE_DIRS} DIRECTORY)

# Generate header included from lm_test project
configure_file("test/test_python.h.in" "${CMAKE_CURRENT_BINARY_DIR}/generated/test_python.h" @ONLY)

# Unit test
set(_PROJECT_NAME lm_test)
add_executable(${_PROJECT_NAME}
    "test/main.cpp"
    "test/test_common.h"
    "test/test_common.cpp"
    "test/test_api.cpp"
    "test/test_comp.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/test_python.h")
target_link_libraries(${_PROJECT_NAME} PRIVATE liblm doctest pybind11::embed lm_test_plugin::interface)
target_include_directories(${_PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
