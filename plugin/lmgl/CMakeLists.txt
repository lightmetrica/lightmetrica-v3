#
#   Lightmetrica - Copyright (c) 2019 Hisanari Otsu
#   Distributed under MIT license. See LICENSE file for details.
#

include(LmAddPlugin)

# Add plugin
lm_add_plugin(
    NAME lmgl
    HEADERS
        "gl.h"
    SOURCES
        "lmgl.cpp"
    LIBRARIES
        gl3w)
