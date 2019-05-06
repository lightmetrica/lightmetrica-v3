#
#   Lightmetrica - Copyright (c) 2019 Hisanari Otsu
#   Distributed under MIT license. See LICENSE file for details.
#

cmake_minimum_required(VERSION 3.10)

# Enable policy to manage VERSION in project() command
cmake_policy(SET CMP0048 NEW)
project(lightmetrica VERSION 3.0.0)

# -----------------------------------------------------------------------------

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

# -----------------------------------------------------------------------------

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

# -----------------------------------------------------------------------------

# Thread
find_package(Threads REQUIRED)

# OpenMP
find_package(OpenMP REQUIRED)

# Directory for in-project external dependencies
set(LM_EXTERNAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external")

# Include dependencies via add_subdirectory
# when the correponding in-project external directory exists.
# Otherwise use find_package via config mode.

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

# libzmq & cppzmq
# https://github.com/zeromq/libzmq
# https://github.com/zeromq/cppzmq
if (LM_USE_EXTERNAL_DIR AND EXISTS "${LM_EXTERNAL_DIR}/libzmq" AND EXISTS "${LM_EXTERNAL_DIR}/cppzmq")
    # libzmq
    option(ENABLE_DRAFTS "" OFF)
    option(BUILD_SHARED "" OFF)
    option(WITH_PERF_TOOL "" OFF)
    option(BUILD_TESTS "" OFF)
    option(ENABLE_CPACK "" OFF)
    # We need to specify EXCLUDE_FROM_ALL to prevent install()
    # command of the subdirectory being called.
    add_subdirectory(external/libzmq EXCLUDE_FROM_ALL)

    # cppzmq
    # We workaround cppzmq's CMakeLists.txt
    # because it internally tries to find ZeroMQ package
    # where we already loaded via add_subdirectory.
    set(_CPPZMQ_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/cppzmq")
    add_library(cppzmq-static INTERFACE)
    target_link_libraries(cppzmq-static INTERFACE libzmq-static)
    target_include_directories(cppzmq-static INTERFACE "${_CPPZMQ_DIR}")
else()
    find_package(ZeroMQ REQUIRED)
    #find_package(cppzmq REQUIRED)

    set(_CPPZMQ_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/cppzmq")
    add_library(cppzmq-static INTERFACE)
    target_link_libraries(cppzmq-static INTERFACE libzmq-static)
    target_include_directories(cppzmq-static INTERFACE "${_CPPZMQ_DIR}")
endif()

# stb
# https://github.com/nothings/stb
add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src/ext/stb")

# OpenGL
find_package(OpenGL REQUIRED)

# gl3w
# https://github.com/skaslev/gl3w
add_library(gl3w STATIC "${CMAKE_CURRENT_SOURCE_DIR}/src/ext/GL/gl3w.c")
target_include_directories(gl3w PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src/ext")
target_link_libraries(gl3w PUBLIC OpenGL::GL)

# GLFW
# https://github.com/glfw/glfw
if (LM_USE_EXTERNAL_DIR AND EXISTS "${LM_EXTERNAL_DIR}/glfw")
    option(GLFW_BUILD_DOCS "" OFF)
    option(GLFW_BUILD_TESTS "" OFF)
    option(GLFW_BUILD_EXAMPLES "" OFF)
    option(GLFW_INSTALL "" OFF)
    add_subdirectory(${LM_EXTERNAL_DIR}/glfw)
else()
    find_package(glfw3 REQUIRED)
endif()

# imgui
# https://github.com/ocornut/imgui
set(_IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/ext/imgui")
add_library(imgui STATIC
    "${_IMGUI_DIR}/imgui.cpp"
    "${_IMGUI_DIR}/imgui_draw.cpp"
    "${_IMGUI_DIR}/imgui_widgets.cpp"
    "${_IMGUI_DIR}/imgui_demo.cpp"
    "${_IMGUI_DIR}/examples/imgui_impl_glfw.cpp"
    "${_IMGUI_DIR}/examples/imgui_impl_opengl3.cpp")
target_link_libraries(imgui PUBLIC glm glfw gl3w)
target_include_directories(imgui PUBLIC
    "${_IMGUI_DIR}"
    "${_IMGUI_DIR}/examples")

# -----------------------------------------------------------------------------

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

# -----------------------------------------------------------------------------

# PCH related
set(_PCH_DIR "${CMAKE_CURRENT_SOURCE_DIR}/pch")
set(_PCH_FILES
    "${_PCH_DIR}/pch.h"
    "${_PCH_DIR}/pch.cpp")

# Main libraries
add_subdirectory(src)

# Plugins
add_subdirectory(plugin)

# Examples
if (LM_BUILD_EXAMPLES)
    add_subdirectory(example)
endif()

# Tests
if (LM_BUILD_TESTS)
    add_subdirectory(test)
    add_subdirectory(pytest)
    add_subdirectory(functest)
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
