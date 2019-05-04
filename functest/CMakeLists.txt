#
#   Lightmetrica - Copyright (c) 2019 Hisanari Otsu
#   Distributed under MIT license. See LICENSE file for details.
#

include(LmAddPlugin)

lm_add_plugin(
    NAME functest_material_visualize_normal
    SOURCES
        "material_visualize_normal.cpp")

lm_add_plugin(
    NAME functest_renderer_ao
    SOURCES
        "renderer_ao.cpp")