#!/bin/bash
# Build the Elsewhere GDExtension adapter
#
# This script compiles the GDExtension shared library.
# Requires:
#   - Godot C++ bindings built at tools/godot-cpp/build/
#   - Elsewhere core compiled (meson build)
#
# Usage:
#   tools/build-elsewhere-extension.sh [build_dir]
#
# The GDExtension is a standalone .so loaded by Godot at runtime.
# No libgodot.so is needed at build time — symbols resolve via
# godot_extension_get_library_symbol() at load time.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${1:-${REPO_ROOT}/build-godot-ext}"

GODOT_CPP="${REPO_ROOT}/tools/godot-cpp"
GODOT_CPP_BUILD="${GODOT_CPP}/build"
ELSEWHERE_BUILD="${REPO_ROOT}/build"

# ─── Check prerequisites ───

if [ ! -f "${GODOT_CPP_BUILD}/bin/libgodot-cpp.linux.template_debug.x86_64.a" ]; then
    echo "ERROR: godot-cpp not built. Run:"
    echo "  cmake -B ${GODOT_CPP_BUILD} -S ${GODOT_CPP} \\"
    echo "    -DGODOTCPP_API_VERSION=4.7 \\"
    echo "    -DGODOTCPP_TARGET=template_debug"
    echo "  cmake --build ${GODOT_CPP_BUILD} -j4"
    exit 1
fi

if [ ! -f "${ELSEWHERE_BUILD}/elsewhere" ]; then
    echo "ERROR: Elsewhere core not built. Run:"
    echo "  meson setup ${ELSEWHERE_BUILD}"
    echo "  meson compile -C ${ELSEWHERE_BUILD}"
    exit 1
fi

# ─── Compile ───

mkdir -p "${BUILD_DIR}"

CXXFLAGS="-I${REPO_ROOT}/src \
          -I${REPO_ROOT}/build \
          -I${REPO_ROOT}/build/src \
          -I${GODOT_CPP}/include \
          -I${GODOT_CPP}/build/gen/include \
          -fPIC \
          -std=c++17 \
          -g \
          -O0 \
          -Wall \
          -Wextra \
          -Wno-unused-parameter \
          -D_GNU_SOURCE \
          -DGDEXTENSION \
          -DLINUX_ENABLED \
          -DUNIX_ENABLED \
          -DTHREADS_ENABLED"

echo "=== Compiling elsewhere_bridge.cpp ==="
g++ ${CXXFLAGS} -c \
    -o "${BUILD_DIR}/elsewhere_bridge.o" \
    "${REPO_ROOT}/src/godot/elsewhere_bridge.cpp" \
    && echo "  OK: elsewhere_bridge.o"

echo "=== Compiling elsewhere_extension.cpp ==="
g++ ${CXXFLAGS} -c \
    -o "${BUILD_DIR}/elsewhere_extension.o" \
    "${REPO_ROOT}/src/godot/elsewhere_extension.cpp" \
    && echo "  OK: elsewhere_extension.o"

echo "=== Linking GDExtension shared library ==="
g++ -shared -o "${BUILD_DIR}/libelsewhere_godot.so" \
    "${BUILD_DIR}/elsewhere_bridge.o" \
    "${BUILD_DIR}/elsewhere_extension.o" \
    -L"${GODOT_CPP_BUILD}/bin" \
    -lgodot-cpp.linux.template_debug.x86_64 \
    -lwayland-server \
    -lwayland-client \
    -lEGL \
    -lGLESv2 \
    -lxkbcommon \
    -ldl \
    -Wl,-rpath,'$ORIGIN' \
    && echo "  OK: libelsewhere_godot.so"

echo ""
echo "=== Build complete ==="
echo "Extension: ${BUILD_DIR}/libelsewhere_godot.so"
echo ""
echo "To use in Godot:"
echo "  1. Copy libelsewhere_godot.so to your project's bin/ directory"
echo "  2. Ensure extension.toml is alongside it"
echo "  3. Enable the extension in Project Settings > General > Extensions"
echo ""
echo "Godot engine: ${REPO_ROOT}/tools/godot-4.7.2-stable/bin/godot.linuxbsd.template_debug.dev.x86_64"
