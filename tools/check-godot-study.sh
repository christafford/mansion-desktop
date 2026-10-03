#!/usr/bin/env bash
# Rendered study/controller and real terminal output checks; requires a display.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
GODOT="${GODOT_BINARY:-$ROOT/tools/Godot_v4.7.2-stable_linux.x86_64}"
"$ROOT/tools/validate-godot-project.sh"
LOG_DIR=$(mktemp -d "${TMPDIR:-/tmp}/mansion-study-check.XXXXXX")
echo "Study check logs: $LOG_DIR"
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
run controller 'STUDY_SMOKE .*failures=0' --script res://tests/study_smoke.gd -- --no-terminal
run color 'SCREEN_COLOR_OK failures=0' --script res://tests/screen_color.gd
run terminal 'LIVE_TERMINAL_OK .*sessions=2 failures=0' --script res://tests/live_terminal.gd -- --terminal-demo
echo "Study checks passed; inspect docs/evidence/live-terminal for visual acceptance."
