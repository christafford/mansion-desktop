#!/bin/sh
# P4-T01: room-render — room geometry with floor and walls visible.
#
# In room mode the camera is inside a 10×5×10 room:
#   - brown floor at y=0
#   - grey walls at z=±5 and x=±5
#   - ceiling at y=5
# The camera at (0, 2.5, 3) with pitch≈-10° looks toward the front wall.
# Bottom-centre pixel hits the floor (brown), top-centre hits the front wall (grey).
. "$(dirname "$0")/lib.sh"

SCE="$(mktemp /tmp/screenshot.XXXXXX.ppm)"

# Compositor in room-camera mode.
start_compositor --room-camera --exit-after-ms 3000 --screenshot "$SCE"

# Client connects and commits a buffer.
run_client --toplevel --buffer 200x100 --color ff0000 --exit-after-ms 2000

expect_line "$WORK/client.out" "^connected$"
expect_line "$WORK/client.out" "^configure"

wait_compositor
[ "$COMPOSITOR_STATUS" -eq 0 ] || fail "compositor exit status $COMPOSITOR_STATUS"

[ -f "$SCE" ] || fail "screenshot file not created"

# Bottom-centre pixel should be floor (brown: 0.38, 0.28, 0.18, ±0.08)
python3 "$(dirname "$0")/ppm_pixel.py" "$SCE" 512 767 61472c 40 || \
    fail "bottom-centre pixel should be brown floor"

# Top-centre pixel should be front wall (grey: 0.50, 0.50, 0.55, ±0.08)
python3 "$(dirname "$0")/ppm_pixel.py" "$SCE" 512 1 7f7f8c 40 || \
    fail "top-centre pixel should be grey wall"

echo "PASS: room-render"
