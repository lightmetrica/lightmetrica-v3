#
#   Lightmetrica - Copyright (c) 2019 Hisanari Otsu
#   Distributed under MIT license. See LICENSE file for details.
#

# Include and source directories
set(_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include/lm")
set(_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

# -----------------------------------------------------------------------------

# Main library
set(_PROJECT_NAME liblm)
set(_HEADER_FILES 
    "${_INCLUDE_DIR}/lm.h"
    "${_INCLUDE_DIR}/user.h"
    "${_INCLUDE_DIR}/logger.h"
    "${_INCLUDE_DIR}/progress.h"
    "${_INCLUDE_DIR}/exception.h"
    "${_INCLUDE_DIR}/parallel.h"
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
    "${_INCLUDE_DIR}/json.h"
    "${_INCLUDE_DIR}/common.h"
    "${_INCLUDE_DIR}/component.h")
set(_SOURCE_FILES 
    "${_SOURCE_DIR}/component.cpp"
    "${_SOURCE_DIR}/user.cpp"
    "${_SOURCE_DIR}/assets.cpp"
    "${_SOURCE_DIR}/scene.cpp"
    "${_SOURCE_DIR}/exception.cpp"
    "${_SOURCE_DIR}/logger.cpp"
    "${_SOURCE_DIR}/progress.cpp"
    "${_SOURCE_DIR}/parallel/parallel.cpp"
    "${_SOURCE_DIR}/parallel/parallel_openmp.cpp"
    "${_SOURCE_DIR}/model/model_wavefrontobj.cpp"
    "${_SOURCE_DIR}/mesh/mesh_raw.cpp"
    "${_SOURCE_DIR}/camera/camera_pinhole.cpp"
    "${_SOURCE_DIR}/light/light_area.cpp"
    "${_SOURCE_DIR}/texture/texture_bitmap.cpp"
    "${_SOURCE_DIR}/material/material_diffuse.cpp"
    "${_SOURCE_DIR}/material/material_glass.cpp"
    "${_SOURCE_DIR}/material/material_glossy.cpp"
    "${_SOURCE_DIR}/material/material_mirror.cpp"
    "${_SOURCE_DIR}/material/material_mask.cpp"
    "${_SOURCE_DIR}/material/material_proxy.cpp"
    "${_SOURCE_DIR}/film/film_bitmap.cpp"
    "${_SOURCE_DIR}/accel/accel_sahbvh.cpp"
    "${_SOURCE_DIR}/renderer/renderer_blank.cpp"
    "${_SOURCE_DIR}/renderer/renderer_raycast.cpp"
    "${_SOURCE_DIR}/renderer/renderer_pt.cpp"
    "${_SOURCE_DIR}/renderer/renderer_ptdirect.cpp"
    "${_SOURCE_DIR}/ext/rang.hpp")
add_library(${_PROJECT_NAME} SHARED ${_HEADER_FILES} ${_SOURCE_FILES} ${_PCH_FILES})
source_group(TREE ${_INCLUDE_DIR} PREFIX "Header Files" FILES ${_HEADER_FILES})
source_group(TREE ${_SOURCE_DIR}  PREFIX "Source Files" FILES ${_SOURCE_FILES})
if (MSVC)
    add_precompiled_header(${_PROJECT_NAME} "${_PCH_DIR}/pch.h" SOURCE_CXX "${_PCH_DIR}/pch.cpp")
endif()
target_link_libraries(${_PROJECT_NAME}
    PUBLIC nlohmann_json glm fmt::fmt-header-only cereal
    PRIVATE OpenMP::OpenMP_CXX
            $<$<CXX_COMPILER_ID:GNU>:stdc++fs>
            $<$<CXX_COMPILER_ID:GNU>:atomic>)
target_include_directories(${_PROJECT_NAME}
    PUBLIC  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
    PRIVATE "${_PCH_DIR}")
target_compile_definitions(${_PROJECT_NAME} PRIVATE -DLM_EXPORTS)
target_compile_features(${_PROJECT_NAME} PUBLIC cxx_std_17)
# Enable warning level 4, treat warning as errors, enable SEH
target_compile_options(${_PROJECT_NAME} PUBLIC $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /EHa>)
# Enable parallel build
target_compile_options(${_PROJECT_NAME} PUBLIC $<$<CXX_COMPILER_ID:MSVC>:/MP>)
# Floating point behavior
target_compile_options(${_PROJECT_NAME} PUBLIC $<$<CXX_COMPILER_ID:MSVC>:/fp:strict /fp:except>)
# filesystem
target_compile_options(${_PROJECT_NAME} PUBLIC $<$<CXX_COMPILER_ID:GNU>:-lstdc++fs>)
set_target_properties(${_PROJECT_NAME} PROPERTIES PREFIX "")
set_target_properties(${_PROJECT_NAME} PROPERTIES FOLDER "lm/lib")
set_target_properties(${_PROJECT_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# -----------------------------------------------------------------------------

# Python binding
set(_PROJECT_NAME pylm)
set(_HEADER_FILES 
    "${_INCLUDE_DIR}/pylm.h")
set(_SOURCE_FILES 
    "${_SOURCE_DIR}/pylm.cpp")
add_library(${_PROJECT_NAME} MODULE ${_HEADER_FILES} ${_SOURCE_FILES})
source_group(TREE ${_INCLUDE_DIR} PREFIX "Header Files" FILES ${_HEADER_FILES})
source_group(TREE ${_SOURCE_DIR}  PREFIX "Source Files" FILES ${_SOURCE_FILES})
target_link_libraries(${_PROJECT_NAME} PRIVATE liblm pybind11::module)
set_target_properties(${_PROJECT_NAME} PROPERTIES PREFIX "${PYTHON_MODULE_PREFIX}" SUFFIX "${PYTHON_MODULE_EXTENSION}")
set_target_properties(${_PROJECT_NAME} PROPERTIES FOLDER "lm/lib")
set_target_properties(${_PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
