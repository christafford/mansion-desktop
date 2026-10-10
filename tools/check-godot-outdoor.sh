#!/usr/bin/env bash
# Bounded default outdoor startup; graphical captures still require inspection.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
GODOT="${GODOT_BINARY:-$ROOT/tools/Godot_v4.7.2-stable_linux.x86_64}"
export SDL_JOYSTICK_LINUX_CLASSIC="${SDL_JOYSTICK_LINUX_CLASSIC:-1}"
"$ROOT/tools/validate-godot-project.sh"
LOG_DIR=$(mktemp -d "${TMPDIR:-/tmp}/elsewhere-outdoor-check.XXXXXX")
echo "Outdoor check logs: $LOG_DIR"
export ELSEWHERE_LAUNCHER_STATE_DIR="$ROOT/.tools/outdoor-launcher-check-$$"
for mode in headless graphical; do
    args=()
    if [[ "$mode" == headless ]]; then args+=(--headless); fi
    if ! timeout -k 2s 60s "$GODOT" --path "$ROOT/world" --audio-driver Dummy \
        --max-fps 60 "${args[@]}" --script res://tests/outdoor_startup.gd > "$LOG_DIR/$mode.log" 2>&1; then
        cat "$LOG_DIR/$mode.log"
        exit 1
    fi
    cat "$LOG_DIR/$mode.log"
    if rg -n 'ERROR:|crashed with signal|Leaked instance|ObjectDB instances leaked' "$LOG_DIR/$mode.log" || \
        ! rg -q '^OUTDOOR_STARTUP_OK .* boundaries=4 failures=0$' "$LOG_DIR/$mode.log"; then
        echo "Outdoor check failed: $mode" >&2
        exit 1
    fi
done
echo "Outdoor startup checks passed; inspect .tools/outdoor-startup-test/*.png."
