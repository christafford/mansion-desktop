#!/usr/bin/env bash
# Prove standard godot-cpp loading independently of the unfinished compositor.
set -euo pipefail
ulimit -c 0
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
GODOT="${GODOT_BINARY:-$ROOT/tools/Godot_v4.7.2-stable_linux.x86_64}"
CPP="$ROOT/tools/godot-cpp"
BUILD="$ROOT/build-godot-probe"
PROJECT="$ROOT/tests/godot-binding"
[[ -x "$GODOT" && -f "$CPP/CMakeLists.txt" ]] || {
    echo "Missing local Godot or godot-cpp sources; see docs/TOOLCHAIN.md" >&2
    exit 1
}
# Use a clean generated binding tree. The old tools/godot-cpp/build headers
# were inconsistent with its checkout and must not be mixed into this target.
if [[ ! -f "$BUILD/CMakeCache.txt" ]]; then
    cmake -S "$PROJECT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Debug
fi
cmake --build "$BUILD" --parallel 2
# Exercise discovery from a fresh cache, not a previously imported project.
PROBE_PROJECT=$(mktemp -d "$BUILD/project.XXXXXX")
mkdir -p "$PROBE_PROJECT/bin"
cp "$PROJECT/project.godot" "$PROJECT/probe.gdextension" "$PROJECT/smoke.gd" "$PROJECT/lifecycle.gd" "$PROJECT/frames.gd" "$PROBE_PROJECT/"
cp "$PROJECT/bin/libmansion_probe.so" "$PROBE_PROJECT/bin/"
PROJECT="$PROBE_PROJECT"
run() {
    local label=$1
    shift
    if ! timeout -k 2s 30s "$GODOT" --headless --audio-driver Dummy --path "$PROJECT" "$@" > "$BUILD/$label.log" 2>&1; then
        cat "$BUILD/$label.log"
        exit 1
    fi
    if rg -n 'ERROR:|crashed with signal|Leaked instance|ObjectDB instances leaked' "$BUILD/$label.log"; then
        echo "Binding check failed; see $BUILD/$label.log" >&2
        exit 1
    fi
}
# The local editor's asynchronous docs loader can outlive a one-frame import.
# Pace startup before teardown; retain error detection and the hard timeout.
run import --import --quit-after 180 --max-fps 60
for attempt in 1 2 3; do
    run "smoke-$attempt" --script res://smoke.gd
    rg -q '^BINDING_SMOKE_OK:' "$BUILD/smoke-$attempt.log"
done
"$BUILD/runtime-lifecycle" "$BUILD/runtime-client"
RUNTIME_BASE=$(mktemp -d /tmp/mansion-godot.XXXXXX)
# Remove only the owned empty root; the runtime must clean its own sockets.
trap 'rmdir "$RUNTIME_BASE"' EXIT
run lifecycle --max-fps 120 --script res://lifecycle.gd -- "$BUILD/runtime-client" "$RUNTIME_BASE"
rg -q '^GODOT_LIFECYCLE_OK .*failures=0$' "$BUILD/lifecycle.log"
cat "$BUILD/lifecycle.log"
"$BUILD/frame-snapshot" "$BUILD/frame-client"
run frames --max-fps 120 --script res://frames.gd -- "$BUILD/frame-client" "$RUNTIME_BASE"
rg -q '^GODOT_FRAMES_OK .*failures=0$' "$BUILD/frames.log"
cat "$BUILD/frames.log"
"$GODOT" --version
git -C "$CPP" rev-parse HEAD
printf 'Binding and lifecycle checks passed: 300 probe calls and 32 protocol trials plus 16 frame/client trials. Logs: %s\n' "$BUILD"
