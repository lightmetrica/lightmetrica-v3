#
#   Lightmetrica - Copyright (c) 2019 Hisanari Otsu
#   Distributed under MIT license. See LICENSE file for details.
#

include(LmAddPlugin)

if (embree_FOUND)
    lm_add_plugin(
        NAME accel_embree
        INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
        LIBRARIES embree
        SOURCES
            "accel_embree.cpp")
endif()