#!/usr/bin/env bash
# Mansion Desktop — Godot project validation
# P21-T01: Verify the Godot project scaffold loads without errors.
#
# Usage:
#   tools/validate-godot-project.sh          # run with editor binary
#   tools/validate-godot-project.sh --help   # show usage
#
# Exit codes:
#   0 — project validated successfully
#   1 — validation failed or Godot binary not found

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR/../world"
GODOT_BIN=""

# Locate the Godot editor binary.
if [ -n "${GODOT_BINARY:-}" ]; then
	GODOT_BIN="$GODOT_BINARY"
elif [ -f "$SCRIPT_DIR/Godot_v4.7.2-stable_linux.x86_64" ]; then
	GODOT_BIN="$SCRIPT_DIR/Godot_v4.7.2-stable_linux.x86_64"
elif [ -f "$SCRIPT_DIR/../tools/Godot_v4.7.2-stable_linux.x86_64" ]; then
	GODOT_BIN="$SCRIPT_DIR/../tools/Godot_v4.7.2-stable_linux.x86_64"
fi

if [ -z "$GODOT_BIN" ]; then
	echo "ERROR: Godot editor binary not found."
	echo "  Set GODOT_BINARY=<path> or place it in tools/"
	exit 1
fi

if [ ! -x "$GODOT_BIN" ]; then
	echo "ERROR: $GODOT_BIN is not executable."
	exit 1
fi

if [ ! -d "$PROJECT_DIR" ]; then
	echo "ERROR: Project directory not found: $PROJECT_DIR"
	exit 1
fi

if [ ! -f "$PROJECT_DIR/project.godot" ]; then
	echo "ERROR: $PROJECT_DIR/project.godot not found."
	exit 1
fi

echo "Validating Godot project: $PROJECT_DIR"
echo "Godot binary: $GODOT_BIN"
echo ""

# --headless --editor does not work; instead use --import
# to validate the project can be imported without errors.
echo "--- Import validation ---"
# Use --quit-after 1 to avoid audio device initialization (no runtime needed)
# The import must succeed with exit code 0
if ! "$GODOT_BIN" --import --quit-after 1 "$PROJECT_DIR" 2>&1 | tee /tmp/godot-import.log; then
	echo "ERROR: Import validation failed."
	exit 1
fi

echo "Import validation passed."

echo ""
echo "--- Scene file syntax check ---"
# Verify the main scene file exists and has content.
SCENE="$PROJECT_DIR/scenes/main.tscn"
if [ ! -f "$SCENE" ]; then
	echo "ERROR: Main scene not found: $SCENE"
	exit 1
fi

SCENE_SIZE=$(wc -c < "$SCENE")
if [ "$SCENE_SIZE" -lt 100 ]; then
	echo "ERROR: Main scene file too small ($SCENE_SIZE bytes)."
	exit 1
fi

echo "Main scene: $SCENE ($SCENE_SIZE bytes)"

# Verify key nodes exist in the scene file.
for node in "Camera3D" "Room" "Floor" "MonitorSlot" "SpawnMarker"; do
	if grep -q "name=\"$node\"" "$SCENE"; then
		echo "  ✓ Scene node: $node"
	else
		echo "  ✗ Missing scene node: $node"
		exit 1
	fi
done

# Verify the autoload script exists.
SCRIPT="$PROJECT_DIR/scripts/game_world.gd"
if [ -f "$SCRIPT" ]; then
	SCRIPT_SIZE=$(wc -c < "$SCRIPT")
	echo "  ✓ Game world script: $SCRIPT ($SCRIPT_SIZE bytes)"
else
	echo "  ✗ Missing game world script: $SCRIPT"
	exit 1
fi

echo ""
echo "Godot project scaffold validated successfully."
exit 0
