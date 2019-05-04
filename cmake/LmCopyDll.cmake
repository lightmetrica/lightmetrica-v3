#
#   Lightmetrica - Copyright (c) 2019 Hisanari Otsu
#   Distributed under MIT license. See LICENSE file for details.
#

#
# CopyDLL
# 
# This module provides automatic copy functions of dynamic libraries
# for Visual Studio build environment in Windows.
#

if(WIN32)
	include(CMakeParseArguments)

    #
    # add_custom_command_copy_dll
    #
    # A custom command to copy a dynamic library
    # to the same directory as a library or an executable.
    #
    # Params
    # - NAME
    #     + Library or executable name.
    #       DLL files would be copied output directory of the specified library or executable.
    # - DLL
    #     + Target DLL file
    #
    function(add_custom_command_copy_dll)
        cmake_parse_arguments(_ARG "" "TARGET;NAME;DLL" "" ${ARGN})
        add_custom_command(
            TARGET ${_ARG_TARGET}
            PRE_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${_ARG_DLL}
                    $<TARGET_FILE_DIR:${_ARG_NAME}>)
    endfunction()

	#
    # add_custom_command_copy_dll_release_debug
    #
    # A custom command to copy a dynamic library
    # to the same directory as a library or an executable.
    # If you want to separate release and debug dlls, use this function
    #
    # Params
    # - NAME
    #     + Library or executable name.
    #       DLL files would be copied output directory of the specified library or executable.
    # - DLL_RELEASE
    #     + Target DLL file (release)
    # - DLL_DEBUG
    #     + Target DLL file (debug)
    #
    function(add_custom_command_copy_dll_release_debug)
        cmake_parse_arguments(_ARG "" "TARGET;NAME;DLL_RELEASE;DLL_DEBUG" "" ${ARGN})
        add_custom_command(
            TARGET ${_ARG_TARGET}
            PRE_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<$<CONFIG:release>:${_ARG_DLL_RELEASE}>$<$<CONFIG:relwithdebinfo>:${_ARG_DLL_RELEASE}>$<$<CONFIG:debug>:${_ARG_DLL_DEBUG}>"
                    "$<TARGET_FILE_DIR:${_ARG_NAME}>")
    endfunction()
endif()