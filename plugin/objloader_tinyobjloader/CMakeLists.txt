#
#   Lightmetrica - Copyright (c) 2019 Hisanari Otsu
#   Distributed under MIT license. See LICENSE file for details.
#

include(LmAddPlugin)

if (LM_USE_EXTERNAL_DIR AND EXISTS "${LM_EXTERNAL_DIR}/tinyobjloader")
    set(_TINYOBJLOADER_DIR "${LM_EXTERNAL_DIR}/tinyobjloader")
elseif (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tinyobjloader")
    set(_TINYOBJLOADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tinyobjloader")
endif()

if (DEFINED _TINYOBJLOADER_DIR)
    add_library(tinyobjloader INTERFACE)
    target_include_directories(tinyobjloader INTERFACE "${_TINYOBJLOADER_DIR}")
    lm_add_plugin(
        NAME objloader_tinyobjloader
        LIBRARIES tinyobjloader
        SOURCES
            "objloader_tinyobjloader.cpp")
endif()
