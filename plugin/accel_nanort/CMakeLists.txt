#
#   Lightmetrica - Copyright (c) 2019 Hisanari Otsu
#   Distributed under MIT license. See LICENSE file for details.
#

include(LmAddPlugin)

if (LM_USE_EXTERNAL_DIR AND EXISTS "${LM_EXTERNAL_DIR}/nanort")
    set(_NANORT_DIR "${LM_EXTERNAL_DIR}/nanort")
elseif (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/nanort")
    set(_NANORT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/nanort")
endif()

if (DEFINED _NANORT_DIR)
    add_library(nanort INTERFACE)
    target_include_directories(nanort INTERFACE "${_NANORT_DIR}")
    lm_add_plugin(
        NAME accel_nanort
        LIBRARIES nanort
        SOURCES
            "accel_nanort.cpp")
endif()