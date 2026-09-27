#!/bin/sh
# P3-T04: present-fullsize — in Application mode the focused surface is
# drawn 2D, scaled to fit the host window (fullscreen).
#
# Flow:
#   1. Client connects, commits a green buffer.
#   2. Input script switches to Application mode → flat_mode=true.
#   3. Compositor renders the focused surface fullscreen.
#   4. Screenshot shows the entire viewport filled with green.
. "$(dirname "$0")/lib.sh"

SCE="$(mktemp /tmp/screenshot.XXXXXX.ppm)"

# Compositor in flat mode (full-size path).  Exits after 8000 ms and takes
# a screenshot during shutdown.
start_compositor --flat --exit-after-ms 8000 --screenshot "$SCE"

# Client draws a green buffer and stays alive for 5 s.
run_client --toplevel --buffer 200x100 --color 00ff00 --exit-after-ms 5000

expect_line "$WORK/client.out" "^connected$"
expect_line "$WORK/client.out" "^configure 800 600$"

wait_compositor
[ "$COMPOSITOR_STATUS" -eq 0 ] || fail "compositor exit status $COMPOSITOR_STATUS"

# Screenshot file exists
[ -f "$SCE" ] || fail "screenshot file not created"

# In Application mode the focused surface is rendered fullscreen (flat_mode).
# The 200×100 green buffer is stretched to fill the 1024×768 viewport.
# The pixel at the centre should be green.
python3 "$(dirname "$0")/ppm_pixel.py" "$SCE" 512 384 00ff00 40 || \
    fail "pixel (512,384) should be green (fullscreen app surface)"

rm -f "$SCE"
echo "PASS present-fullsize"
