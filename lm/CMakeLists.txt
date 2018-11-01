#
#   Lightmetrica - Copyright (c) 2018 Hisanari Otsu
#   Distributed under MIT license. See LICENSE file for details.
#

cmake_minimum_required(VERSION 3.12)
project(lm)

# -----------------------------------------------------------------------------

# CMake module path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# PCH support
include(PrecompiledHeader)

# Use project filters in Visual Studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

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

# cereal
option(JUST_INSTALL_CEREAL "" ON)
add_subdirectory(external/cereal)

# -----------------------------------------------------------------------------

# Include and source directories
set(_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/lm")
set(_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

# PCH related
set(_PCH_DIR "${CMAKE_CURRENT_SOURCE_DIR}/pch")
set(_PCH_FILES
    "${_PCH_DIR}/pch.h"
    "${_PCH_DIR}/pch.cpp")

# -----------------------------------------------------------------------------

# Main library
set(_PROJECT_NAME liblm)
set(_HEADER_FILES 
    "${_INCLUDE_DIR}/lm.h"
    "${_INCLUDE_DIR}/api.h"
    "${_INCLUDE_DIR}/logger.h"
    "${_INCLUDE_DIR}/math.h"
    "${_INCLUDE_DIR}/assets.h"
    "${_INCLUDE_DIR}/mesh.h"
    "${_INCLUDE_DIR}/camera.h"
    "${_INCLUDE_DIR}/texture.h"
    "${_INCLUDE_DIR}/material.h"
    "${_INCLUDE_DIR}/light.h"
    "${_INCLUDE_DIR}/scene.h"
    "${_INCLUDE_DIR}/accel.h"
    "${_INCLUDE_DIR}/film.h"
    "${_INCLUDE_DIR}/model.h"
    "${_INCLUDE_DIR}/renderer.h"
    "${_INCLUDE_DIR}/detail/common.h"
    "${_INCLUDE_DIR}/detail/forward.h"
    "${_INCLUDE_DIR}/detail/json.h"
    "${_INCLUDE_DIR}/detail/component.h"
    "${_INCLUDE_DIR}/detail/parallel.h")
set(_SOURCE_FILES 
    "${_SOURCE_DIR}/component.cpp"
    "${_SOURCE_DIR}/logger.cpp"
    "${_SOURCE_DIR}/api.cpp"
    "${_SOURCE_DIR}/assets.cpp"
    "${_SOURCE_DIR}/scene.cpp"
    "${_SOURCE_DIR}/model/model_wavefrontobj.cpp"
    "${_SOURCE_DIR}/mesh/mesh_raw.cpp"
    "${_SOURCE_DIR}/camera/camera_pinhole.cpp"
    "${_SOURCE_DIR}/texture/texture_bitmap.cpp"
    "${_SOURCE_DIR}/film/film_bitmap.cpp"
    "${_SOURCE_DIR}/accel/accel_sahbvh.cpp"
    "${_SOURCE_DIR}/renderer/renderer_blank.cpp"
    "${_SOURCE_DIR}/renderer/renderer_raycast.cpp"
    "${_SOURCE_DIR}/renderer/renderer_pt.cpp")
add_library(${_PROJECT_NAME} SHARED ${_HEADER_FILES} ${_SOURCE_FILES} ${_PCH_FILES})
source_group(TREE ${_INCLUDE_DIR} PREFIX "Header Files" FILES ${_HEADER_FILES})
source_group(TREE ${_SOURCE_DIR}  PREFIX "Source Files" FILES ${_SOURCE_FILES})
add_precompiled_header(${_PROJECT_NAME} "${_PCH_DIR}/pch.h" SOURCE_CXX "${_PCH_DIR}/pch.cpp")
target_link_libraries(${_PROJECT_NAME}
    PUBLIC nlohmann_json glm fmt-header-only cereal
    PRIVATE spdlog)
target_include_directories(${_PROJECT_NAME}
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    PRIVATE "${_PCH_DIR}")
target_compile_definitions(${_PROJECT_NAME} PRIVATE -DLM_EXPORTS)
target_compile_features(${_PROJECT_NAME} PUBLIC cxx_std_17)
set_target_properties(${_PROJECT_NAME} PROPERTIES FOLDER "lm/lib")
set_target_properties(${_PROJECT_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# -----------------------------------------------------------------------------

# Python binding
set(_PROJECT_NAME pylm)
set(_HEADER_FILES 
    "${_INCLUDE_DIR}/pylm.h"
    "${_INCLUDE_DIR}/detail/pylm_component.h"
    "${_INCLUDE_DIR}/detail/pylm_json.h")
set(_SOURCE_FILES 
    "${_SOURCE_DIR}/pylm.cpp")
add_library(${_PROJECT_NAME} MODULE ${_HEADER_FILES} ${_SOURCE_FILES})
source_group(TREE ${_INCLUDE_DIR} PREFIX "Header Files" FILES ${_HEADER_FILES})
source_group(TREE ${_SOURCE_DIR}  PREFIX "Source Files" FILES ${_SOURCE_FILES})
target_link_libraries(${_PROJECT_NAME} PRIVATE liblm pybind11::module)
set_target_properties(${_PROJECT_NAME} PROPERTIES PREFIX "${PYTHON_MODULE_PREFIX}" SUFFIX "${PYTHON_MODULE_EXTENSION}")
set_target_properties(${_PROJECT_NAME} PROPERTIES FOLDER "lm/lib")
set_target_properties(${_PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# -----------------------------------------------------------------------------

# Examples
add_subdirectory(example)

# Tests
add_subdirectory(test)
add_subdirectory(pytest)
