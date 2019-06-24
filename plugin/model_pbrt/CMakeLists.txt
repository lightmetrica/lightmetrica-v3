#
#   Lightmetrica - Copyright (c) 2019 Hisanari Otsu
#   Distributed under MIT license. See LICENSE file for details.
#

include(LmAddPlugin)

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pbrt-parser")
    set(_PBRT_PARSER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/pbrt-parser")
endif()

if (DEFINED _PBRT_PARSER_DIR)
    add_subdirectory(pbrt-parser EXCLUDE_FROM_ALL)
    target_include_directories(pbrtParser_static PUBLIC "${_PBRT_PARSER_DIR}/pbrtParser/include")
    lm_add_plugin(
        NAME model_pbrt
        LIBRARIES pbrtParser_static
        SOURCES
            "model_pbrt.cpp")
endif()
