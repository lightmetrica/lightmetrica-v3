#
#   Lightmetrica - Copyright (c) 2019 Hisanari Otsu
#   Distributed under MIT license. See LICENSE file for details.
#

# Helper function to add an example
function(lm_add_example)
    # Parse arguments
    cmake_parse_arguments(_ARG "" "NAME" "SOURCES;LIBRARIES" ${ARGN})

    # Executable
    add_executable(${_ARG_NAME} ${_ARG_SOURCES})
    target_link_libraries(${_ARG_NAME} PRIVATE liblm ${_ARG_LIBRARIES})
    set_target_properties(${_ARG_NAME} PROPERTIES FOLDER "lm/example")
    set_target_properties(${_ARG_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    source_group("Source Files" FILES ${_ARG_SOURCES})
endfunction()

# -------------------------------------------------------------------------------------------------

# Non-GUI examples
lm_add_example(NAME blank SOURCES "blank.cpp")
lm_add_example(NAME quad SOURCES "quad.cpp")
lm_add_example(NAME raycast SOURCES "raycast.cpp")
lm_add_example(NAME pt SOURCES "pt.cpp")
lm_add_example(NAME custom_material SOURCES "custom_material.cpp")
lm_add_example(NAME custom_renderer SOURCES "custom_renderer.cpp")

# GUI examples
if (LM_BUILD_GUI_EXAMPLES)
    # Create a target for imgui
    set(_IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/imgui")
    if (EXISTS ${_IMGUI_DIR})
        add_library(imgui STATIC
            "${_IMGUI_DIR}/imgui.cpp"
            "${_IMGUI_DIR}/imgui_draw.cpp"
            "${_IMGUI_DIR}/imgui_widgets.cpp"
            "${_IMGUI_DIR}/imgui_demo.cpp"
            "${_IMGUI_DIR}/examples/imgui_impl_glfw.cpp"
            "${_IMGUI_DIR}/examples/imgui_impl_opengl3.cpp")
        target_link_libraries(imgui PUBLIC glm glfw GLEW::GLEW OpenGL::GL)
        target_include_directories(imgui PUBLIC
            "${_IMGUI_DIR}"
            "${_IMGUI_DIR}/examples")
        target_compile_definitions(imgui PUBLIC -DIMGUI_IMPL_OPENGL_LOADER_GLEW)    

        # Interface target for lmgl.h
        add_library(lmgl INTERFACE)
        target_include_directories(lmgl INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/lmgl")
        target_link_libraries(lmgl INTERFACE imgui)

        # Add GUI examples
        lm_add_example(NAME interactive SOURCES "interactive.cpp" LIBRARIES lmgl)
        # lm_add_example(NAME debugio_vis_server SOURCES "lmgl.h" "debugio_vis_server.cpp" LIBRARIES imgui)
        # lm_add_example(NAME debugio_vis SOURCES "debugio_vis.cpp")
    else()
        message(FATAL_ERROR "Missing imgui directory. Didn't you forget to update submodule?")
    endif()
endif()