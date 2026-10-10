#!/usr/bin/env bash
# Import and run the selected project. Godot sometimes exits zero on script errors.
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="${1:-$SCRIPT_DIR/../world}"
GODOT_BIN="${GODOT_BINARY:-$SCRIPT_DIR/Godot_v4.7.2-stable_linux.x86_64}"
if [[ "$PROJECT_DIR" == --help ]]; then
    echo "Usage: tools/validate-godot-project.sh [project-directory]"
    exit 0
fi
[[ -x "$GODOT_BIN" && -f "$PROJECT_DIR/project.godot" ]] || {
    echo "Missing executable Godot binary or project.godot" >&2; exit 1;
}
if [[ "$(realpath "$PROJECT_DIR")" == "$(realpath "$SCRIPT_DIR/../world")" ]]; then
    "$SCRIPT_DIR/build-godot-runtime.sh"
fi
LOG_DIR=$(mktemp -d "${TMPDIR:-/tmp}/elsewhere-godot-check.XXXXXX")
echo "Godot validation logs: $LOG_DIR"
check() {
    local phase=$1
    shift
    if ! timeout -k 2s 180s "$GODOT_BIN" --path "$PROJECT_DIR" --headless --audio-driver Dummy "$@" > "$LOG_DIR/$phase.log" 2>&1; then
        cat "$LOG_DIR/$phase.log"
        echo "Godot $phase failed" >&2
        exit 1
    fi
    if rg -n 'SCRIPT ERROR:|(^|[[:space:]])ERROR:|Parse Error|Failed to load|crashed with signal' "$LOG_DIR/$phase.log"; then
        echo "Godot $phase reported errors; see $LOG_DIR/$phase.log" >&2
        exit 1
    fi
}
# This engine queues editor documentation callbacks during first import.
# Allow deferred work to drain before shutdown (see recovery handoff).
check import --import --quit-after 180 --max-fps 60
check runtime --quit-after 5
echo "Godot import and runtime checks passed (not visual acceptance)."
