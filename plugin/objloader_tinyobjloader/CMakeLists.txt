#
#   Lightmetrica - Copyright (c) 2019 Hisanari Otsu
#   Distributed under MIT license. See LICENSE file for details.
#

include(LmAddPlugin)

if (LM_USE_EXTERNAL_DIR AND EXISTS "${LM_EXTERNAL_DIR}/tinyobjloader")
    add_library(tinyobjloader INTERFACE)
    target_include_directories(tinyobjloader INTERFACE "${LM_EXTERNAL_DIR}/tinyobjloader")
    lm_add_plugin(
        NAME objloader_tinyobjloader
        LIBRARIES tinyobjloader
        SOURCES
            "objloader_tinyobjloader.cpp")
endif()
