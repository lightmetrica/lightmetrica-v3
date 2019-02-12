#
#   Lightmetrica - Copyright (c) 2018 Hisanari Otsu
#   Distributed under MIT license. See LICENSE file for details.
#

# Add a plugin
function(lm_add_plugin)
    # Parse arguments
    cmake_parse_arguments(_ARG "" "NAME;INCLUDE_DIR" "HEADERS;SOURCES;LIBRARIES" ${ARGN})

    # INTERFACE library for headers
    if (DEFINED _ARG_INCLUDE_DIR)
        set(_INTERFACE_DEFINED 1)
        add_library(${_ARG_NAME}_interface INTERFACE)
        add_library(${_ARG_NAME}::interface ALIAS ${_ARG_NAME}_interface)
        target_include_directories(${_ARG_NAME}_interface
            INTERFACE "$<BUILD_INTERFACE:${_ARG_INCLUDE_DIR}>")
    else()
        set(_INTERFACE_DEFINED 0)
    endif()

    # MODULE library for the dynamic loaded library
    add_library(${_ARG_NAME} MODULE ${_ARG_HEADERS} ${_ARG_SOURCES})
    target_link_libraries(${_ARG_NAME}
        PRIVATE
            liblm
            $<${_INTERFACE_DEFINED}:${_ARG_NAME}_interface>
            ${_ARG_LIBRARIES})
    set_target_properties(${_ARG_NAME} PROPERTIES PREFIX "")
    set_target_properties(${_ARG_NAME} PROPERTIES FOLDER "lm/plugin")
    set_target_properties(${_ARG_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    source_group("Header Files" FILES ${_ARG_HEADERS})
    source_group("Source Files" FILES ${_ARG_SOURCES})
endfunction()
