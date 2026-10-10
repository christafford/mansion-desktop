#!/usr/bin/env bash
# Elsewhere — Run the Godot world frontend.
# Launch the study. The legacy C++ executable remains a separate frontend.
#
# Usage:
#   tools/run-godot.sh                  # run the study
#   tools/run-godot.sh --editor         # open the editor
#   tools/run-godot.sh --headless       # run without GUI (for CI)
#
# Environment:
#   GODOT_BINARY=<path>  override the default binary location
#   SDL_JOYSTICK_LINUX_CLASSIC=0  opt back into SDL's evdev joystick backend

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
# The pinned engine can crash in SDL's evdev device sort before opening a window
# on Steam Deck. Classic joystick enumeration avoids that sysfs-ordering path;
# keyboard/mouse input still comes from the focused host window. Respect an
# explicit override. See docs/handoffs/27-godot-startup.md for address-level proof.
export SDL_JOYSTICK_LINUX_CLASSIC="${SDL_JOYSTICK_LINUX_CLASSIC:-1}"
exec "$GODOT_BIN" --path "$PROJECT_DIR" "$@"
