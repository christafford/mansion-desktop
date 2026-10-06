#!/usr/bin/env bash
# Rendered study/controller and real terminal output checks; requires a display.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
GODOT="${GODOT_BINARY:-$ROOT/tools/Godot_v4.7.2-stable_linux.x86_64}"
"$ROOT/tools/validate-godot-project.sh"
LOG_DIR=$(mktemp -d "${TMPDIR:-/tmp}/mansion-study-check.XXXXXX")
echo "Study check logs: $LOG_DIR"
export MANSION_LAUNCHER_STATE_DIR="$ROOT/.tools/study-launcher-check-$$"
python3 "$ROOT/tests/test_desktop_apps.py"
python3 "$ROOT/tests/test_godot_launcher.py"
run() {
    local label=$1 marker=$2
    shift 2
    if ! timeout -k 2s 45s "$GODOT" --path "$ROOT/world" --audio-driver Dummy --max-fps 60 "$@" > "$LOG_DIR/$label.log" 2>&1; then
        cat "$LOG_DIR/$label.log"
        exit 1
    fi
    cat "$LOG_DIR/$label.log"
    if rg -n 'ERROR:|crashed with signal|Leaked instance|ObjectDB instances leaked' "$LOG_DIR/$label.log" || ! rg -q "$marker" "$LOG_DIR/$label.log"; then
        echo "Study check failed: $label" >&2
        exit 1
    fi
}
run transforms 'SCREEN_TRANSFORM_OK cases=8 failures=0' --headless --script res://tests/screen_transform.gd
run keyboard-map 'KEYBOARD_MAP_OK cases=27 failures=0' --headless --script res://tests/keyboard_map.gd
run pointer-map 'POINTER_MAP_OK .*failures=0' --headless --script res://tests/pointer_map.gd
run controller 'STUDY_SMOKE .*failures=0' --script res://tests/study_smoke.gd -- --no-terminal
run furniture 'FURNITURE_DRAG_OK .*failures=0' --script res://tests/furniture_drag.gd -- --no-terminal
run details 'STUDY_DETAILS_OK .*failures=0' --script res://tests/study_details.gd
run color 'SCREEN_COLOR_OK failures=0' --script res://tests/screen_color.gd
run terminal 'LIVE_TERMINAL_OK .*sessions=2 failures=0' --script res://tests/live_terminal.gd -- --terminal-demo
run terminal-input 'TERMINAL_INPUT_OK .*failures=0' --script res://tests/terminal_input.gd
run terminal-pointer 'TERMINAL_POINTER_OK .*failures=0' --script res://tests/terminal_pointer.gd
run terminal-resize 'TERMINAL_RESIZE_OK .*failures=0' --script res://tests/terminal_resize.gd
run launcher 'APP_LAUNCHER_OK .*failures=0' --script res://tests/app_launcher.gd
run application-objects 'APPLICATION_OBJECTS_OK .*failures=0' --script res://tests/application_objects.gd
run application-transition 'APPLICATION_TRANSITION_OK failures=0' --script res://tests/application_transition.gd
run application-gravity 'APPLICATION_GRAVITY_OK .*failures=0' --script res://tests/application_gravity.gd
echo "Study checks passed; inspect .tools/terminal-output-test, .tools/terminal-input-test, .tools/terminal-pointer-test, .tools/terminal-resize-test and .tools/launcher-test-captures."
