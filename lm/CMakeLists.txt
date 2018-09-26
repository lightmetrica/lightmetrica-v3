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

	# Sources
    "${_SOURCE_DIR}/api.cpp"
)

target_link_libraries(${_PROJECT_NAME} PUBLIC nlohmann_json glm)
target_include_directories(${_PROJECT_NAME} PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")
target_compile_definitions(${_PROJECT_NAME} PRIVATE -DLM_EXPORTS)

# -----------------------------------------------------------------------------

# Python binding
set(_PROJECT_NAME pylm)
add_library(${_PROJECT_NAME} MODULE "src/pylm.cpp")
target_link_libraries(${_PROJECT_NAME} PRIVATE liblm pybind11::module)
set_target_properties(${_PROJECT_NAME} PROPERTIES PREFIX "${PYTHON_MODULE_PREFIX}" SUFFIX "${PYTHON_MODULE_EXTENSION}")

# -----------------------------------------------------------------------------

# Unit test
set(_PROJECT_NAME lm_test)
add_executable(${_PROJECT_NAME} "test/main.cpp")
target_link_libraries(${_PROJECT_NAME} PRIVATE liblm doctest)
