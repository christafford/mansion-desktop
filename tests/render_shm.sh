#!/bin/sh
# P1-T05: render-shm — the compositor draws a shm buffer as a GL texture
# and --screenshot produces a PPM where pixel (100,50) is red (the client
# buffer) and pixel (600,500) is the compositor clear colour.
. "$(dirname "$0")/lib.sh"

SCE="$(mktemp /tmp/screenshot.XXXXXX.ppm)"

# Compositor exits after 3000 ms and takes a screenshot during shutdown.
# Client exits after 2000 ms, so it disconnects and prints "done" before
# the compositor takes the screenshot (the orphaned-surface mechanism
# keeps the surface alive for the screenshot).
start_compositor --exit-after-ms 3000 --screenshot "$SCE"
run_client --buffer 200x100 --color ff0000 --exit-after-ms 2000

# Client connected and completed its lifecycle
expect_line "$WORK/client.out" "^connected$"
expect_line "$WORK/client.out" "^configure 800 600$"
expect_line "$WORK/client.out" "^frame [0-9]+$"
expect_line "$WORK/client.out" "^release$"
expect_line "$WORK/client.out" "^done$"

# Compositor has already exited (exit-after-ms 3000), wait for it.
wait_compositor
[ "$COMPOSITOR_STATUS" -eq 0 ] || fail "compositor exit status $COMPOSITOR_STATUS"

# Screenshot file exists
[ -f "$SCE" ] || fail "screenshot file not created"

# The client buffer is 200×100 at origin (0,0).
# Vertex shader maps (0,0) → clip top-left, (200,100) → lower-right.
# glClearColor is (0.15, 0.15, 0.2) ≈ (38, 38, 51).
# PPM Y-flip means rows 0-99 are the buffer area, cols 0-199.

# Red at (100, 50) — inside the buffer
python3 "$(dirname "$0")/ppm_pixel.py" "$SCE" 100 50 ff0000 40 || fail "pixel (100,50) should be red"

# Clear color at (600, 500) — outside buffer
python3 "$(dirname "$0")/ppm_pixel.py" "$SCE" 600 500 262633 20 || fail "pixel (600,500) should be clear color"

echo "PASS render-shm"
rm -f "$SCE"
