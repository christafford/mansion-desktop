#!/usr/bin/env bash
# Build the verified standard binding and stage it for the study.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/build-godot-probe"
if [[ ! -f "$BUILD/CMakeCache.txt" ]]; then
    cmake -S "$ROOT/tests/godot-binding" -B "$BUILD" -DCMAKE_BUILD_TYPE=Debug
fi
cmake --build "$BUILD" --target elsewhere_probe --parallel 2
DEST="$ROOT/world/addons/elsewhere_runtime/libelsewhere_runtime.so"
# Atomic replacement leaves already-running Godot processes' mapping intact.
cp "$ROOT/tests/godot-binding/bin/libelsewhere_probe.so" "$DEST.tmp"
mv "$DEST.tmp" "$DEST"
