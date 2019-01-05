#
#   Lightmetrica - Copyright (c) 2019 Hisanari Otsu
#   Distributed under MIT license. See LICENSE file for details.
#

# Examples using Lightmetrica as an external library
cmake_minimum_required(VERSION 3.10)
project(lightmetrica_examples LANGUAGES CXX)

find_package(lightmetrica REQUIRED)

add_executable(blank "../blank.cpp")
target_link_libraries(blank PRIVATE lightmetrica::liblm)
add_executable(quad "../quad.cpp")
target_link_libraries(quad PRIVATE lightmetrica::liblm)
add_executable(raycast "../raycast.cpp")
target_link_libraries(raycast PRIVATE lightmetrica::liblm)
