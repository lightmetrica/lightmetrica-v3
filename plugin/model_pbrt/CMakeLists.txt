#
#   Lightmetrica - Copyright (c) 2019 Hisanari Otsu
#   Distributed under MIT license. See LICENSE file for details.
#

include(LmAddPlugin)

find_package(pbrtParser)
if (pbrtParser_FOUND)
    lm_add_plugin(
        NAME model_pbrt
        LIBRARIES pbrtParser::pbrtParser
        SOURCES
            "model_pbrt.cpp")
endif()
