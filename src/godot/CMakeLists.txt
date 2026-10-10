# CMakeLists.txt for the Elsewhere GDExtension
#
# Builds the GDExtension shared library that Godot loads.
# Requires:
#   - godot-cpp built at ${GODOT_CPP_DIR}/build
#   - Elsewhere core library at ${ELSEWHERE_BUILD_DIR}
#   - libgodot.so for linking (from a full Godot build)
#
# Usage:
#   cmake -B build-godot -S . \
#     -DCMAKE_BUILD_TYPE=Debug \
#     -DGODOT_CPP_DIR=/path/to/godot-cpp/build \
#     -DELSEWHERE_BUILD_DIR=/path/to/elsewhere/build \
#     -DLIBGODOT=/path/to/libgodot.so
#   cmake --build build-godot

cmake_minimum_required(VERSION 3.16)
project(elsewhere-godot-extension LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─── Paths (set by caller) ───

if(NOT DEFINED GODOT_CPP_DIR)
    message(FATAL_ERROR "GODOT_CPP_DIR not set. Pass -DGODOT_CPP_DIR=...")
endif()

if(NOT DEFINED ELSEWHERE_BUILD_DIR)
    message(FATAL_ERROR "ELSEWHERE_BUILD_DIR not set. Pass -DELSEWHERE_BUILD_DIR=...")
endif()

if(NOT DEFINED LIBGODOT)
    message(FATAL_ERROR "LIBGODOT not set. Pass -DLIBGODOT=/path/to/libgodot.so")
endif()

# ─── godot-cpp ───

include_directories(
    ${GODOT_CPP_DIR}/include
    ${GODOT_CPP_DIR}/include/core
    ${GODOT_CPP_DIR}/include/variant
    ${GODOT_CPP_DIR}/gen/include
)

# ─── Elsewhere core ───

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# ─── Sources ───

set(ELSEWHERE_EXTENSION_SOURCES
    elsewhere_extension.cpp
    elsewhere_bridge.cpp
)

# ─── Build the extension ───

add_library(elsewhere_godot SHARED ${ELSEWHERE_EXTENSION_SOURCES})

target_link_libraries(elsewhere_godot PRIVATE
    ${GODOT_CPP_DIR}/bin/libgodot-cpp.linux.template_debug.x86_64.a
    ${LIBGODOT}
)

# Link against the elsewhere core shared library if available,
# otherwise link the static objects directly
set(ELSEWHERE_CORE_LIB ${ELSEWHERE_BUILD_DIR}/libelsewhere-core.a)
if(EXISTS ${ELSEWHERE_CORE_LIB})
    target_link_libraries(elsewhere_godot PRIVATE ${ELSEWHERE_CORE_LIB})
else()
    # Fallback: link the elsewhere executable (which includes core + display)
    # This is not ideal but works for testing
    message(WARNING "Using elsewhere executable as fallback link")
    set(ELSEWHERE_EXE ${ELSEWHERE_BUILD_DIR}/elsewhere)
    if(EXISTS ${ELSEWHERE_EXE})
        target_link_libraries(elsewhere_godot PRIVATE ${ELSEWHERE_EXE})
    endif()
endif()

# ─── Output ───

set_target_properties(elsewhere_godot PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../bin
    POSITION_INDEPENDENT_CODE ON
)

# ─── Godot extension metadata ───

# Generate extension.toml for Godot to find the library
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/extension_api.json.in
    ${CMAKE_CURRENT_SOURCE_DIR}/../bin/extension_api.json
    @ONLY
)

message(STATUS "GDExtension output: ${CMAKE_CURRENT_SOURCE_DIR}/../bin/libelsewhere_godot.so")
