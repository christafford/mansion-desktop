#!/bin/sh
# P3-T04: present-fullsize — in Application mode the focused surface is
# drawn 2D, scaled to fit the host window (fullscreen).
#
# Flow:
#   1. Client connects, commits a green buffer.
#   2. Input script switches to Application mode → flat_mode=false,
#      which triggers render_application_fullscreen().
#   3. Compositor renders the focused surface fullscreen.
#   4. Screenshot shows the entire viewport filled with green.
. "$(dirname "$0")/lib.sh"

SCRIPT="$WORK/input-script.txt"
SCE="$(mktemp /tmp/screenshot.XXXXXX.ppm)"

# Build the input script:
#   1. Wait for client to connect and commit its buffer.
#   2. Switch to Application mode (sets flat_mode=false → fullscreen path).
#   3. Wait for a few frames to render.
#   4. Quit.
cat > "$SCRIPT" <<EOF
wait 1500
mode app
wait 500
quit
EOF

# Compositor in flat mode (full-size path).  Exits after 8000 ms and takes
# a screenshot during shutdown.
start_compositor --flat --exit-after-ms 8000 --screenshot "$SCE" --input-script "$SCRIPT"

# Client draws a green buffer and stays alive for 5 s.
run_client --toplevel --buffer 200x100 --color 00ff00 --exit-after-ms 5000

expect_line "$WORK/client.out" "^connected$"
expect_line "$WORK/client.out" "^configure 800 600$"

wait_compositor
[ "$COMPOSITOR_STATUS" -eq 0 ] || fail "compositor exit status $COMPOSITOR_STATUS"

# Screenshot file exists
[ -f "$SCE" ] || fail "screenshot file not created"

# In Application mode the focused surface is rendered fullscreen.
# The 200×100 green buffer is stretched to fill the 1024×768 viewport.
# The pixel at the centre should be green.
python3 "$(dirname "$0")/ppm_pixel.py" "$SCE" 512 384 00ff00 40 || \
    fail "pixel (512,384) should be green (fullscreen app surface)"

rm -f "$SCE"
echo "PASS present-fullsize"
