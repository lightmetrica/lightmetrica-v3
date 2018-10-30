#
#   Lightmetrica - Copyright (c) 2018 Hisanari Otsu
#   Distributed under MIT license. See LICENSE file for details.
#

# Add an example
function(lm_add_example)
    # Parse arguments
    cmake_parse_arguments(_ARG "" "NAME" "SOURCES" ${ARGN})

    # Executable
    add_executable(${_ARG_NAME} ${_ARG_SOURCES})
    target_link_libraries(${_ARG_NAME} PRIVATE liblm)
    set_target_properties(${_ARG_NAME} PROPERTIES FOLDER "lm/example")
    set_target_properties(${_ARG_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    source_group("Source Files" FILES ${_ARG_SOURCES})
endfunction()

# -----------------------------------------------------------------------------

lm_add_example(NAME 001_blank_image SOURCES "001_blank_image.cpp")
lm_add_example(NAME 002_render_quad SOURCES "002_render_quad.cpp")
