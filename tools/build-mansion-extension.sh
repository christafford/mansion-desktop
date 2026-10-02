#!/bin/bash
# Build the Mansion GDExtension adapter
#
# This script compiles the GDExtension shared library.
# Requires:
#   - Godot C++ bindings built at tools/godot-cpp/build/
#   - libgodot.so from a full Godot engine build (for full GDExtension)
#   - Mansion core compiled (meson build)
#
# Usage:
#   tools/build-mansion-extension.sh [build_dir]
#
# Without libgodot.so, this script verifies compilation of the
# bridge and extension code (syntax/type checking).
#
# Full GDExtension linking requires libgodot.so and the mansion
# core as a static library.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${1:-${REPO_ROOT}/build-godot-ext}"

GODOT_CPP="${REPO_ROOT}/tools/godot-cpp"
GODOT_CPP_BUILD="${GODOT_CPP}/build"
MANSION_BUILD="${REPO_ROOT}/build"

# ─── Check prerequisites ───

if [ ! -f "${GODOT_CPP_BUILD}/bin/libgodot-cpp.linux.template_debug.x86_64.a" ]; then
    echo "ERROR: godot-cpp not built. Run:"
    echo "  cmake -B ${GODOT_CPP_BUILD} -S ${GODOT_CPP} \\"
    echo "    -DGODOTCPP_API_VERSION=4.7 \\"
    echo "    -DGODOTCPP_TARGET=template_debug"
    echo "  cmake --build ${GODOT_CPP_BUILD} -j4"
    exit 1
fi

if [ ! -f "${MANSION_BUILD}/mansion-desktop" ]; then
    echo "ERROR: Mansion core not built. Run:"
    echo "  meson setup ${MANSION_BUILD}"
    echo "  meson compile -C ${MANSION_BUILD}"
    exit 1
fi

# ─── Locate libgodot.so ───

LIBGODOT=""
if [ -n "${MANSION_GODOT_SO:-}" ]; then
    LIBGODOT="${MANSION_GODOT_SO}"
else
    for candidate in \
        "${REPO_ROOT}/tools/godot-4.7.2-stable/bin/libgodot.linuxbsd.template_debug.x86_64.so" \
        "/usr/lib/libgodot.so" \
        "/usr/lib64/libgodot.so" \
        "/usr/local/lib/libgodot.so"; do
        if [ -f "$candidate" ]; then
            LIBGODOT="$candidate"
            break
        fi
    done
fi

HAS_GODOT_SO=false
if [ -n "${LIBGODOT}" ] && [ -f "${LIBGODOT}" ]; then
    HAS_GODOT_SO=true
    echo "Found libgodot.so: ${LIBGODOT}"
else
    echo "WARNING: libgodot.so not found. Building bridge-only (no GDExtension linking)."
    echo "Set MANSION_GODOT_SO=/path/to/libgodot.so to link the full extension."
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

echo "=== Compiling mansion_bridge.cpp ==="
g++ ${CXXFLAGS} -c \
    -o "${BUILD_DIR}/mansion_bridge.o" \
    "${REPO_ROOT}/src/godot/mansion_bridge.cpp" \
    && echo "  OK: mansion_bridge.o"

echo "=== Compiling mansion_extension.cpp ==="
g++ ${CXXFLAGS} -c \
    -o "${BUILD_DIR}/mansion_extension.o" \
    "${REPO_ROOT}/src/godot/mansion_extension.cpp" \
    && echo "  OK: mansion_extension.o"

echo "=== Compilation complete ==="
echo ""
if [ "${HAS_GODOT_SO}" = true ]; then
    echo "Full GDExtension linking requires:"
    echo "  1. libgodot.so (from full Godot engine build)"
    echo "  2. Mansion core as a static library (modify meson.build)"
    echo ""
    echo "To link manually:"
    echo "  g++ -shared -o ${BUILD_DIR}/libmansion_godot.so \\"
    echo "    ${BUILD_DIR}/mansion_extension.o \\"
    echo "    ${BUILD_DIR}/mansion_bridge.o \\"
    echo "    -L${GODOT_CPP_BUILD}/bin \\"
    echo "    -lgodot-cpp.linux.template_debug.x86_64 \\"
    echo "    -L${MANSION_BUILD} -lmansion-core \\"
    echo "    -lwayland-server -lwayland-client \\"
    echo "    -lEGL -lGLESv2 -lxkbcommon -ldl \\"
    echo "    -Wl,-rpath,'\\\$ORIGIN'"
else
    echo "Bridge objects compiled successfully."
    echo "Full GDExtension will link when libgodot.so is available."
fi

echo ""
echo "=== Build complete ==="
