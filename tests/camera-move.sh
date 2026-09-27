#!/bin/sh
# P2-T06: camera-move — WASD/arrow keys move the camera,
# right button held enables mouse look.
#
# Acceptance: moving the camera forward (closer to the panel) makes
# the projected panel appear larger, verified by pixel count.
#
# Camera:   (0, 0, Z, 0, 0) looking at origin (panel at 0,0,0)
# Panel:    width=2, height=1.5 (set by --camera)
# Viewport: 1024 × 768
#
# At Z=10 the panel height projects to ~145 px.
# At Z=5  the panel height projects to ~290 px.

. "$(dirname "$0")/lib.sh"

# Evdev keycodes for WASD (linux/input-event-codes.h).
KEY_W=17
KEY_A=30
KEY_S=31
KEY_D=32
KEY_UP=103
KEY_DOWN=108
KEY_LEFT=105
KEY_RIGHT=106

SCRA="$(mktemp /tmp/screenA.XXXXXX.ppm)"
SCRB="$(mktemp /tmp/screenB.XXXXXX.ppm)"

# ── Run A: camera at Z=10 ───────────────────────────────────────
start_compositor --camera 0,0,10,0,0 --exit-after-ms 3000 --screenshot "$SCRA"

run_client --toplevel --buffer 200x200 --color ff0000 --exit-after-ms 2000

wait_compositor
[ "$COMPOSITOR_STATUS" -eq 0 ] || fail "compositor A exit status $COMPOSITOR_STATUS"

# ── Run B: move forward by 5 (W key, 500 ms) → Z=5 ─────────────
# Rebuild the socket dir so the second run doesn't collide.
rm -rf "$XDG_RUNTIME_DIR"
mkdir -m 700 "$XDG_RUNTIME_DIR"
COMPOSITOR_PID=""

# Build an input script: press W, wait 500 ms, release W, quit.
SCRIPT="$WORK/input-script.txt"
cat > "$SCRIPT" <<EOF
wait 100
key $KEY_W press
wait 500
key $KEY_W release
quit
EOF

start_compositor --camera 0,0,10,0,0 --exit-after-ms 3000 --screenshot "$SCRB" --input-script "$SCRIPT"

run_client --toplevel --buffer 200x200 --color ff0000 --exit-after-ms 2000

wait_compositor
[ "$COMPOSITOR_STATUS" -eq 0 ] || fail "compositor B exit status $COMPOSITOR_STATUS"

# ── Verify: panel is larger in run B ────────────────────────────
# Count red pixels in column 512 (panel centre).
RED_COUNT_A=$(python3 -c "
import sys
with open('$SCRA','rb') as f:
    f.readline()  # magic P6
    line = f.readline().strip()
    while line.startswith(b'#'): line = f.readline().strip()
    w, h = map(int, line.split())
    f.readline()  # skip maxval
    data = f.read()
count = 0
for y in range(h):
    idx = (y*w + 512)*3
    r = data[idx]; g = data[idx+1]; b = data[idx+2]
    if r > 200 and g < 50 and b < 50:
        count += 1
print(count)
")

RED_COUNT_B=$(python3 -c "
import sys
with open('$SCRB','rb') as f:
    f.readline()  # magic P6
    line = f.readline().strip()
    while line.startswith(b'#'): line = f.readline().strip()
    w, h = map(int, line.split())
    f.readline()  # skip maxval
    data = f.read()
count = 0
for y in range(h):
    idx = (y*w + 512)*3
    r = data[idx]; g = data[idx+1]; b = data[idx+2]
    if r > 200 and g < 50 and b < 50:
        count += 1
print(count)
")

echo "Red pixel count at Z=10: $RED_COUNT_A" >&2
echo "Red pixel count at Z=5:  $RED_COUNT_B" >&2

# At Z=5 the panel should be noticeably taller.
# Minimum expected: ~200 red pixels (Z=5 projected height ≈ 290 px).
[ "$RED_COUNT_B" -gt "$RED_COUNT_A" ] || fail "panel at Z=5 ($RED_COUNT_B) not taller than at Z=10 ($RED_COUNT_A)"

echo "PASS camera-move"
rm -f "$SCRA" "$SCRB"
