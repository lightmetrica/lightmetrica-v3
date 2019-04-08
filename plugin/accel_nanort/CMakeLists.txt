#
#   Lightmetrica - Copyright (c) 2019 Hisanari Otsu
#   Distributed under MIT license. See LICENSE file for details.
#

include(LmAddPlugin)

if (LM_USE_EXTERNAL_DIR AND EXISTS "${LM_EXTERNAL_DIR}/nanort")
    add_library(nanort INTERFACE)
    target_include_directories(nanort INTERFACE "${LM_EXTERNAL_DIR}/nanort")
    lm_add_plugin(
        NAME accel_nanort
        INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
        LIBRARIES nanort
        SOURCES
            "accel_nanort.cpp")
endif()