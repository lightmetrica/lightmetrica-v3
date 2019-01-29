#
#   Lightmetrica - Copyright (c) 2019 Hisanari Otsu
#   Distributed under MIT license. See LICENSE file for details.
#

# Add an example
function(lm_add_example)
    # Parse arguments
    cmake_parse_arguments(_ARG "" "NAME" "SOURCES; LIBRARIES" ${ARGN})

    # Executable
    add_executable(${_ARG_NAME} ${_ARG_SOURCES})
    target_link_libraries(${_ARG_NAME} PRIVATE liblm ${_ARG_LIBRARIES})
    set_target_properties(${_ARG_NAME} PROPERTIES FOLDER "lm/example")
    set_target_properties(${_ARG_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    source_group("Source Files" FILES ${_ARG_SOURCES})
endfunction()

# -----------------------------------------------------------------------------

lm_add_example(NAME blank SOURCES "blank.cpp")
lm_add_example(NAME quad SOURCES "quad.cpp")
lm_add_example(NAME raycast SOURCES "raycast.cpp")
lm_add_example(NAME pt SOURCES "pt.cpp")
lm_add_example(NAME custom_material SOURCES "custom_material.cpp")
lm_add_example(NAME custom_renderer SOURCES "custom_renderer.cpp")
lm_add_example(NAME serialization SOURCES "serialization.cpp")
lm_add_example(NAME save_loading_time SOURCES "save_loading_time.cpp")
lm_add_example(NAME update_asset SOURCES "update_asset.cpp")
lm_add_example(NAME walk_components SOURCES "walk_components.cpp")
lm_add_example(NAME logger_usage SOURCES "logger_usage.cpp")
lm_add_example(NAME error_handling SOURCES "error_handling.cpp")
lm_add_example(NAME interactive SOURCES "interactive.cpp")

