#
#   Lightmetrica - Copyright (c) 2018 Hisanari Otsu
#   Distributed under MIT license. See LICENSE file for details.
#

cmake_minimum_required(VERSION 3.12)
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
set(_HEADER_FILES
    "${_INCLUDE_DIR}/lm.h"
    "${_INCLUDE_DIR}/api.h"
    "${_INCLUDE_DIR}/logger.h")
set(_DETAIL_HEADER_FILES
    "${_INCLUDE_DIR}/detail/common.h"
    "${_INCLUDE_DIR}/detail/comp.h"
    "${_INCLUDE_DIR}/detail/serial.h")
set(_SOURCE_FILES
    "${_SOURCE_DIR}/comp.cpp"
    "${_SOURCE_DIR}/logger.cpp"
    "${_SOURCE_DIR}/api.cpp")    
add_library(${_PROJECT_NAME} SHARED ${_HEADER_FILES} ${_DETAIL_HEADER_FILES} ${_SOURCE_FILES})
target_link_libraries(${_PROJECT_NAME}
    PUBLIC nlohmann_json glm fmt-header-only
    PRIVATE spdlog)
target_include_directories(${_PROJECT_NAME}
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")
target_compile_definitions(${_PROJECT_NAME} PRIVATE -DLM_EXPORTS)
target_compile_features(${_PROJECT_NAME} PUBLIC cxx_std_17)
source_group("Header Files" FILES ${_HEADER_FILES})
source_group("Header Files\\detail" FILES ${_DETAIL_HEADER_FILES})
source_group("Source Files" FILES ${_SOURCE_FILES})

# -----------------------------------------------------------------------------

# Python binding
set(_PROJECT_NAME pylm)
set(_DETAIL_HEADER_FILES
    "${_INCLUDE_DIR}/detail/pylm.h")
set(_SOURCE_FILES
    "${_SOURCE_DIR}/pylm.cpp")
add_library(${_PROJECT_NAME} MODULE ${_DETAIL_HEADER_FILES} ${_SOURCE_FILES})
target_link_libraries(${_PROJECT_NAME} PRIVATE liblm pybind11::module)
set_target_properties(${_PROJECT_NAME} PROPERTIES
    PREFIX "${PYTHON_MODULE_PREFIX}"
    SUFFIX "${PYTHON_MODULE_EXTENSION}")
source_group("Header Files\\detail" FILES ${_DETAIL_HEADER_FILES})
source_group("Source Files" FILES ${_SOURCE_FILES})

# -----------------------------------------------------------------------------

# Add a plugin
function(lm_add_plugin)
    # Parse arguments
    cmake_parse_arguments(_ARG "" "NAME;INCLUDE_DIR" "HEADERS;SOURCES" ${ARGN})

    # INTERFACE library for headers
    add_library(${_ARG_NAME}_interface INTERFACE)
    add_library(${_ARG_NAME}::interface ALIAS ${_ARG_NAME}_interface)
    target_include_directories(${_ARG_NAME}_interface
        INTERFACE "$<BUILD_INTERFACE:${_ARG_INCLUDE_DIR}>")

    # MODULE library for the dynamic loaded library
    add_library(${_ARG_NAME} MODULE ${_ARG_HEADERS} ${_ARG_SOURCES})
    target_link_libraries(${_ARG_NAME} PRIVATE liblm ${_ARG_NAME}_interface)
    source_group("Header Files" FILES ${_ARG_HEADERS})
    source_group("Source Files" FILES ${_ARG_SOURCES})
endfunction()

# -----------------------------------------------------------------------------

# Plugin for unit test
lm_add_plugin(
    NAME lm_test_plugin
    INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    HEADERS
        "test/test_interface.h"
    SOURCES
        "test/test_interface_impl.cpp")

# Get python root directory
find_package(Python REQUIRED COMPONENTS Development)
get_filename_component(_PYTHON_ROOT ${Python_INCLUDE_DIRS} DIRECTORY)

# Generate header included from lm_test project
configure_file("test/test_python.h.in" "${CMAKE_CURRENT_BINARY_DIR}/generated/test_python.h" @ONLY)

# Unit test
set(_PROJECT_NAME lm_test)
set(_HEADER_FILES
    "test/test_common.h"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/test_python.h")
set(_SOURCE_FILES
    "test/main.cpp"
    "test/test_common.cpp"
    "test/test_api.cpp"
    "test/test_comp.cpp"
    "test/test_pylm.cpp")
add_executable(${_PROJECT_NAME} ${_HEADER_FILES} ${_SOURCE_FILES})
target_link_libraries(${_PROJECT_NAME} PRIVATE liblm doctest pybind11::embed lm_test_plugin::interface)
target_include_directories(${_PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
source_group("Header Files" FILES ${_HEADER_FILES})
source_group("Source Files" FILES ${_SOURCE_FILES})