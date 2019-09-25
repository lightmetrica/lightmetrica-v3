#
#   Lightmetrica - Copyright (c) 2019 Hisanari Otsu
#   Distributed under MIT license. See LICENSE file for details.
#

include(LmAddPlugin)

# Find vdbloader library
find_package(vdbloader)
if (vdbloader_FOUND)
    # Create plugin
    lm_add_plugin(
        NAME volume_openvdb
        LIBRARIES
            vdbloader::vdbloader
        SOURCES
            "volume_openvdb.cpp"
            "renderer_volraycast.cpp")
endif()