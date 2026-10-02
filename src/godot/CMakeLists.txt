# CMakeLists.txt for the Mansion GDExtension
#
# Builds the GDExtension shared library that Godot loads.
# Requires:
#   - godot-cpp built at ${GODOT_CPP_DIR}/build
#   - Mansion core library at ${MANSION_BUILD_DIR}
#   - libgodot.so for linking (from a full Godot build)
#
# Usage:
#   cmake -B build-godot -S . \
#     -DCMAKE_BUILD_TYPE=Debug \
#     -DGODOT_CPP_DIR=/path/to/godot-cpp/build \
#     -DMANSION_BUILD_DIR=/path/to/mansion/build \
#     -DLIBGODOT=/path/to/libgodot.so
#   cmake --build build-godot

cmake_minimum_required(VERSION 3.16)
project(mansion-godot-extension LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─── Paths (set by caller) ───

if(NOT DEFINED GODOT_CPP_DIR)
    message(FATAL_ERROR "GODOT_CPP_DIR not set. Pass -DGODOT_CPP_DIR=...")
endif()

if(NOT DEFINED MANSION_BUILD_DIR)
    message(FATAL_ERROR "MANSION_BUILD_DIR not set. Pass -DMANSION_BUILD_DIR=...")
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

# ─── Mansion core ───

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# ─── Sources ───

set(MANSION_EXTENSION_SOURCES
    mansion_extension.cpp
    mansion_bridge.cpp
)

# ─── Build the extension ───

add_library(mansion_godot SHARED ${MANSION_EXTENSION_SOURCES})

target_link_libraries(mansion_godot PRIVATE
    ${GODOT_CPP_DIR}/bin/libgodot-cpp.linux.template_debug.x86_64.a
    ${LIBGODOT}
)

# Link against the mansion core shared library if available,
# otherwise link the static objects directly
set(MANSION_CORE_LIB ${MANSION_BUILD_DIR}/libmansion-core.a)
if(EXISTS ${MANSION_CORE_LIB})
    target_link_libraries(mansion_godot PRIVATE ${MANSION_CORE_LIB})
else()
    # Fallback: link the mansion executable (which includes core + display)
    # This is not ideal but works for testing
    message(WARNING "Using mansion executable as fallback link")
    set(MANSION_EXE ${MANSION_BUILD_DIR}/mansion-desktop)
    if(EXISTS ${MANSION_EXE})
        target_link_libraries(mansion_godot PRIVATE ${MANSION_EXE})
    endif()
endif()

# ─── Output ───

set_target_properties(mansion_godot PROPERTIES
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

message(STATUS "GDExtension output: ${CMAKE_CURRENT_SOURCE_DIR}/../bin/libmansion_godot.so")
