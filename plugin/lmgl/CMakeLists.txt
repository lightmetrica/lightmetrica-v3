#
#   Lightmetrica - Copyright (c) 2019 Hisanari Otsu
#   Distributed under MIT license. See LICENSE file for details.
#

include(LmAddPlugin)

# GL3W
add_library(gl3w STATIC "ext/GL/gl3w.c")
target_include_directories(gl3w PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/ext")
target_link_libraries(gl3w PUBLIC OpenGL::GL)

# Add plugin
lm_add_plugin(
    NAME lmgl
    HEADERS
        "gl.h"
    SOURCES
        "lmgl.cpp"
    LIBRARIES
        gl3w)
