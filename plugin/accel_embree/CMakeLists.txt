#
#   Lightmetrica - Copyright (c) 2019 Hisanari Otsu
#   Distributed under MIT license. See LICENSE file for details.
#

include(LmAddPlugin)

# embree
# https://github.com/embree/embree

find_package(embree 3.0)
if (embree_FOUND)
    # Create embree target
    add_library(embree INTERFACE)
    target_link_libraries(embree INTERFACE "${EMBREE_LIBRARIES}")
    target_include_directories(embree INTERFACE "${EMBREE_INCLUDE_DIRS}")

    # Create plugin
    lm_add_plugin(
        NAME accel_embree
        INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
        LIBRARIES embree
        SOURCES
            "embree.h"
            "accel_embree.cpp"
            "embree.cpp"
            "accel_embree_instanced.cpp")

    # Custom command to copy dynamic libraries
    if (WIN32)
        include(LmCopyDll)
        add_custom_target(copydll_embree)
        add_custom_command_copy_dll(
            TARGET copydll_embree
            NAME accel_embree
            DLL ${embree_DIR}/bin/embree3.dll)
        add_custom_command_copy_dll(
            TARGET copydll_embree
            NAME accel_embree
            DLL ${embree_DIR}/bin/tbb.dll)
        add_custom_command_copy_dll(
            TARGET copydll_embree
            NAME accel_embree
            DLL ${embree_DIR}/bin/tbbmalloc.dll)
        set_target_properties(copydll_embree PROPERTIES FOLDER "lm/plugin")
    endif()
endif()