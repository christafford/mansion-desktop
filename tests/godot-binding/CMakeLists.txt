cmake_minimum_required(VERSION 3.22)
project(mansion_binding_probe LANGUAGES CXX)
set(GODOTCPP_API_VERSION "4.7" CACHE STRING "Pinned local engine API")
set(GODOTCPP_PRECISION "single" CACHE STRING "Engine precision")
set(GODOTCPP_TARGET "template_debug" CACHE STRING "Binding target")
set(GODOTCPP_BUILD_PROFILE "${CMAKE_CURRENT_SOURCE_DIR}/build_profile.json" CACHE FILEPATH "Minimal API")
add_subdirectory("../../tools/godot-cpp" godot-cpp)
add_library(mansion_probe SHARED ../../src/godot/binding_probe.cpp)
target_link_libraries(mansion_probe PRIVATE godot::cpp)
target_compile_options(mansion_probe PRIVATE -Wall -Wextra)
target_link_options(mansion_probe PRIVATE -Wl,--no-undefined)
set_target_properties(mansion_probe PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
