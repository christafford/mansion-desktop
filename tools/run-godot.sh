#!/usr/bin/env bash
# Mansion Desktop — Run the Godot world frontend.
# P21-T01 scaffold: launches the Godot editor with the world project.
#
# Usage:
#   tools/run-godot.sh                  # open Godot editor
#   tools/run-godot.sh --editor         # same (explicit)
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

exec "$GODOT_BIN" --path "$PROJECT_DIR" "$@"
