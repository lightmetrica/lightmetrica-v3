#
#   Lightmetrica - Copyright (c) 2019 Hisanari Otsu
#   Distributed under MIT license. See LICENSE file for details.
#

include(LmAddPlugin)

# Normalize the path separator
file(TO_CMAKE_PATH "$ENV{CONDA_PREFIX}" _CONDA_PREFIX)

# Conda prefix directory
if (MSVC)
    set(_CONDA_LIB_PREFIX "${_CONDA_PREFIX}/Library")
else()
    set(_CONDA_LIB_PREFIX "${_CONDA_PREFIX}")
endif()

# Set module directory for find_package (incl. hints for transitive dependencies)
list(APPEND CMAKE_MODULE_PATH "${_CONDA_LIB_PREFIX}/lib/cmake/OpenVDB")
set(ILMBASE_ROOT "${_CONDA_LIB_PREFIX}")
set(OPENEXR_ROOT "${_CONDA_LIB_PREFIX}")
set(BLOSC_ROOT "${_CONDA_LIB_PREFIX}")

# Find OpenVDB
find_package(OpenVDB)
if (OpenVDB_FOUND)
    # Find dependencies
    find_package(IlmBase REQUIRED)
    find_package(OpenEXR REQUIRED)

    # Create plugin
    lm_add_plugin(
        NAME volume_openvdb
        LIBRARIES
            OpenVDB::openvdb
            OpenEXR::IlmImf
            IlmBase::IlmThread
            IlmBase::Iex
        SOURCES
            "volume_openvdb.cpp"
            "renderer_volraycast.cpp")
endif()