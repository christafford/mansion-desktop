#!/usr/bin/env bash
# Mansion Desktop — Run the Godot world frontend.
# Launch the study. The legacy C++ executable remains a separate frontend.
#
# Usage:
#   tools/run-godot.sh                  # run the study
#   tools/run-godot.sh --editor         # open the editor
#   tools/run-godot.sh --headless       # run without GUI (for CI)
#
# Environment:
#   GODOT_BINARY=<path>  override the default binary location

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR/../world"

# Locate Godot binary.
if [ -n "${GODOT_BINARY:-}" ]; then
	GODOT_BIN="$GODOT_BINARY"
elif [ -f "$SCRIPT_DIR/Godot_v4.7.2-stable_linux.x86_64" ]; then
	GODOT_BIN="$SCRIPT_DIR/Godot_v4.7.2-stable_linux.x86_64"
elif [ -f "$SCRIPT_DIR/../tools/Godot_v4.7.2-stable_linux.x86_64" ]; then
	GODOT_BIN="$SCRIPT_DIR/../tools/Godot_v4.7.2-stable_linux.x86_64"
else
	echo "ERROR: Godot editor binary not found."
	echo "  Set GODOT_BINARY=<path> or place it in tools/"
	exit 1
fi

if [ ! -x "$GODOT_BIN" ]; then
	echo "ERROR: $GODOT_BIN is not executable."
	exit 1
fi

"$SCRIPT_DIR/build-godot-runtime.sh"
exec "$GODOT_BIN" --path "$PROJECT_DIR" "$@"
