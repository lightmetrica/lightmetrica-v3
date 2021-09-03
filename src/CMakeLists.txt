#
#   Lightmetrica - Copyright (c) 2019 Hisanari Otsu
#   Distributed under MIT license. See LICENSE file for details.
#

# Include and source directories
set(_PCH_DIR "${PROJECT_SOURCE_DIR}/pch")
set(_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include/lm")
set(_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

# -------------------------------------------------------------------------------------------------

# Main library
set(_PROJECT_NAME liblm)
set(_HEADER_FILES 
    "${_INCLUDE_DIR}/lm.h"
    "${_INCLUDE_DIR}/version.h"
    "${_INCLUDE_DIR}/user.h"
    "${_INCLUDE_DIR}/logger.h"
    "${_INCLUDE_DIR}/loggercontext.h"
    "${_INCLUDE_DIR}/progress.h"
    "${_INCLUDE_DIR}/progresscontext.h"
    "${_INCLUDE_DIR}/scheduler.h"
    "${_INCLUDE_DIR}/debug.h"
    "${_INCLUDE_DIR}/exception.h"
    "${_INCLUDE_DIR}/parallel.h"
    "${_INCLUDE_DIR}/parallelcontext.h"
    "${_INCLUDE_DIR}/math.h"
    "${_INCLUDE_DIR}/mesh.h"
    "${_INCLUDE_DIR}/camera.h"
    "${_INCLUDE_DIR}/texture.h"
    "${_INCLUDE_DIR}/material.h"
    "${_INCLUDE_DIR}/light.h"
    "${_INCLUDE_DIR}/assetgroup.h"
    "${_INCLUDE_DIR}/scene.h"
	"${_INCLUDE_DIR}/scenenode.h"
    "${_INCLUDE_DIR}/accel.h"
    "${_INCLUDE_DIR}/film.h"
    "${_INCLUDE_DIR}/model.h"
    "${_INCLUDE_DIR}/renderer.h"
    "${_INCLUDE_DIR}/json.h"
    "${_INCLUDE_DIR}/jsontype.h"
    "${_INCLUDE_DIR}/common.h"
    "${_INCLUDE_DIR}/component.h"
    "${_INCLUDE_DIR}/core.h"
    "${_INCLUDE_DIR}/serial.h"
    "${_INCLUDE_DIR}/serialtype.h"
    "${_INCLUDE_DIR}/surface.h"
    "${_INCLUDE_DIR}/objloader.h"
    "${_INCLUDE_DIR}/medium.h"
    "${_INCLUDE_DIR}/phase.h"
    "${_INCLUDE_DIR}/volume.h"
    "${_INCLUDE_DIR}/path.h"
    "${_INCLUDE_DIR}/bidir.h"
    "${_INCLUDE_DIR}/timer.h"
    )
set(_SOURCE_FILES 
    "${_SOURCE_DIR}/component.cpp"
    "${_SOURCE_DIR}/version.cpp"
    "${_SOURCE_DIR}/user.cpp"
    "${_SOURCE_DIR}/assetgroup.cpp"
    "${_SOURCE_DIR}/scene.cpp"
    "${_SOURCE_DIR}/exception.cpp"
    "${_SOURCE_DIR}/logger.cpp"
    "${_SOURCE_DIR}/progress.cpp"
    "${_SOURCE_DIR}/scheduler.cpp"
    "${_SOURCE_DIR}/debug.cpp"
    "${_SOURCE_DIR}/parallel/parallel.cpp"
    "${_SOURCE_DIR}/parallel/parallel_openmp.cpp"
    "${_SOURCE_DIR}/model/model_wavefrontobj.cpp"
    "${_SOURCE_DIR}/objloader/objloader.cpp"
    "${_SOURCE_DIR}/objloader/objloader_simple.cpp"
    "${_SOURCE_DIR}/mesh/mesh_raw.cpp"
    "${_SOURCE_DIR}/mesh/mesh_wavefrontobj.cpp"
    "${_SOURCE_DIR}/camera/camera_pinhole.cpp"
    "${_SOURCE_DIR}/light/light_area.cpp"
    "${_SOURCE_DIR}/light/light_directional.cpp"
    "${_SOURCE_DIR}/light/light_point.cpp"
    "${_SOURCE_DIR}/light/light_env.cpp"
    "${_SOURCE_DIR}/light/light_envconst.cpp"
    "${_SOURCE_DIR}/texture/texture_bitmap.cpp"
    "${_SOURCE_DIR}/texture/texture_constant.cpp"
    "${_SOURCE_DIR}/material/material_diffuse.cpp"
    "${_SOURCE_DIR}/material/material_glass.cpp"
    "${_SOURCE_DIR}/material/material_glossy.cpp"
    "${_SOURCE_DIR}/material/material_mirror.cpp"
    "${_SOURCE_DIR}/material/material_mask.cpp"
    "${_SOURCE_DIR}/material/material_proxy.cpp"
    "${_SOURCE_DIR}/material/material_mixture.cpp"
    "${_SOURCE_DIR}/film/film_bitmap.cpp"
    "${_SOURCE_DIR}/accel/accel_sahbvh.cpp"
    "${_SOURCE_DIR}/renderer/renderer_blank.cpp"
    "${_SOURCE_DIR}/renderer/renderer_raycast.cpp"
    "${_SOURCE_DIR}/renderer/renderer_pt.cpp"
    "${_SOURCE_DIR}/renderer/renderer_lt.cpp"
    "${_SOURCE_DIR}/renderer/renderer_bdpt.cpp"
    "${_SOURCE_DIR}/renderer/renderer_bdptopt.cpp"
    "${_SOURCE_DIR}/renderer/renderer_volpt.cpp"
    "${_SOURCE_DIR}/medium/medium_homogeneous.cpp"
    "${_SOURCE_DIR}/medium/medium_heterogeneous.cpp"
    "${_SOURCE_DIR}/volume/volume_checker.cpp"
    "${_SOURCE_DIR}/volume/volume_constant.cpp"
    "${_SOURCE_DIR}/volume/volume_gaussian.cpp"
    "${_SOURCE_DIR}/volume/volume_multi.cpp"
    "${_SOURCE_DIR}/volume/volume_sphere.cpp"
    "${_SOURCE_DIR}/phase/phase_hg.cpp"
    "${_SOURCE_DIR}/phase/phase_isotropic.cpp"
    "${_SOURCE_DIR}/ext/rang.hpp")
set(_PCH_FILES
    "${_PCH_DIR}/pch.h"
    "${_PCH_DIR}/pch.cpp")
# Add a shared library
add_library(${_PROJECT_NAME} SHARED ${_HEADER_FILES} ${_SOURCE_FILES} ${_PCH_FILES})
# Alias to match the target included by add_subdirectory to the exported target
add_library(lightmetrica::liblm ALIAS liblm)
# Source group for Visual Studio IDE
source_group(TREE ${_INCLUDE_DIR} PREFIX "Header Files" FILES ${_HEADER_FILES})
source_group(TREE ${_SOURCE_DIR}  PREFIX "Source Files" FILES ${_SOURCE_FILES})
# Precompiled header
if (MSVC)
    add_precompiled_header(${_PROJECT_NAME} "${_PCH_DIR}/pch.h" SOURCE_CXX "${_PCH_DIR}/pch.cpp")
endif()
# Configure dependency
target_link_libraries(${_PROJECT_NAME}
    PUBLIC  cereal
            glm
            nlohmann_json
            # fmt::fmt-header-only
    PRIVATE lm_version
            stb::stb_image
            OpenMP::OpenMP_CXX
            $<$<CXX_COMPILER_ID:GNU>:stdc++fs>
            $<$<CXX_COMPILER_ID:GNU>:atomic>)
# Include directories
target_include_directories(${_PROJECT_NAME}
    PUBLIC  # Use generator expression to support different include directories
            # whether the target is exporterd or not.
            "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
            # Use relative directory to make the exported package relocatable
            "$<INSTALL_INTERFACE:include>"
    PRIVATE "${_PCH_DIR}")
target_compile_definitions(${_PROJECT_NAME} PRIVATE -DLM_EXPORTS)
target_compile_definitions(${_PROJECT_NAME} PUBLIC -D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
target_compile_definitions(${_PROJECT_NAME} PUBLIC -D_CRT_SECURE_NO_WARNINGS)
# Use SIMD in glm
target_compile_definitions(${_PROJECT_NAME} PUBLIC -DGLM_FORCE_INTRINSICS)
# Configuration flag
target_compile_definitions(${_PROJECT_NAME} PUBLIC
    $<$<CONFIG:Debug>:LM_USE_CONFIG_DEBUG>
    $<$<CONFIG:Release>:LM_USE_CONFIG_RELEASE>
    $<$<CONFIG:RelWithDebInfo>:LM_USE_CONFIG_RELWITHDEBINFO>)
# Use C++17
target_compile_features(${_PROJECT_NAME} PUBLIC cxx_std_20)
# Enable warning level 4, treat warning as errors, enable SEH
target_compile_options(${_PROJECT_NAME} PUBLIC $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /EHa>)
# Enable parallel build
target_compile_options(${_PROJECT_NAME} PUBLIC $<$<CXX_COMPILER_ID:MSVC>:/MP>)
# Floating point behavior
target_compile_options(${_PROJECT_NAME} PUBLIC $<$<CXX_COMPILER_ID:MSVC>:/fp:strict /fp:except>)
# Big obj file
target_compile_options(${_PROJECT_NAME} PUBLIC $<$<CXX_COMPILER_ID:MSVC>:/bigobj>)
# filesystem
target_compile_options(${_PROJECT_NAME} PUBLIC $<$<CXX_COMPILER_ID:GNU>:-lstdc++fs>)
# Remove `lib` prefix of the output library name in Linux enviroment
set_target_properties(${_PROJECT_NAME} PROPERTIES PREFIX "")
# Add postfix '-debug' for debug build to differentiate both binaries in a distribution
set_target_properties(${_PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "-debug")
# Folder name for Visual Studio IDE where the project is
set_target_properties(${_PROJECT_NAME} PROPERTIES FOLDER "lm/lib")
# Output directories
set_target_properties(${_PROJECT_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
# Add targets to the target group
# Note that install(TARGET) must be written here because
# adding target defined in different directory requires CMake>=3.13.
# https://gitlab.kitware.com/cmake/cmake/merge_requests/2152
if (LM_INSTALL)
    install(
        TARGETS ${_PROJECT_NAME}
        EXPORT ${PROJECT_NAME}Targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

# -------------------------------------------------------------------------------------------------

# Python binding
set(_PROJECT_NAME pylm)
set(_HEADER_FILES 
    "${_INCLUDE_DIR}/pylm.h")
set(_SOURCE_FILES 
    "${_SOURCE_DIR}/pylm.cpp")
set(_PCH_FILES
    "${_PCH_DIR}/pch_pylm.h"
    "${_PCH_DIR}/pch_pylm.cpp")
add_library(${_PROJECT_NAME} MODULE ${_HEADER_FILES} ${_SOURCE_FILES} ${_PCH_FILES})
target_include_directories(${_PROJECT_NAME} PRIVATE "${_PCH_DIR}")
source_group(TREE ${_INCLUDE_DIR} PREFIX "Header Files" FILES ${_HEADER_FILES})
source_group(TREE ${_SOURCE_DIR}  PREFIX "Source Files" FILES ${_SOURCE_FILES})
if (MSVC)
    add_precompiled_header(${_PROJECT_NAME} "${_PCH_DIR}/pch_pylm.h" SOURCE_CXX "${_PCH_DIR}/pch_pylm.cpp")
endif()
target_link_libraries(${_PROJECT_NAME} PRIVATE liblm pybind11::module)
set_target_properties(${_PROJECT_NAME} PROPERTIES PREFIX "${PYTHON_MODULE_PREFIX}" SUFFIX "${PYTHON_MODULE_EXTENSION}")
set_target_properties(${_PROJECT_NAME} PROPERTIES FOLDER "lm/lib")
if (CONDA_BUILD)
    # Output extension modules to the module directory
    set(_PYTHON_MODULE_DIR "${PROJECT_SOURCE_DIR}/lightmetrica")
    set_target_properties(${_PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${_PYTHON_MODULE_DIR}")
else()
    set_target_properties(${_PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()
if (LM_INSTALL)
    install(
        TARGETS ${_PROJECT_NAME}
        EXPORT ${PROJECT_NAME}Targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()