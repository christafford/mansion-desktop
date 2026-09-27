#!/bin/sh
# P2-T05: render-egl — the compositor draws an EGL-rendered client buffer.
# The client uses EGL PBuffer to render green, exports via memfd/shm.
# The compositor (with --egl) skips the ARGB→RGBA swizzle and uploads RGBA directly.
. "$(dirname "$0")/lib.sh"

SCE="$(mktemp /tmp/screenshot.XXXXXX.ppm)"

# Compositor exits after 3000 ms and takes a screenshot during shutdown.
# Client exits after 2000 ms, so it disconnects before the screenshot.
start_compositor --exit-after-ms 3000 --screenshot "$SCE" --egl
run_client --buffer 200x100 --egl --exit-after-ms 2000

# Client connected and completed its lifecycle
expect_line "$WORK/client.out" "^connected$"
expect_line "$WORK/client.out" "^configure 800 600$"
expect_line "$WORK/client.out" "^frame [0-9]+$"
expect_line "$WORK/client.out" "^release$"
expect_line "$WORK/client.out" "^done$"

# Compositor has already exited, wait for it.
wait_compositor
[ "$COMPOSITOR_STATUS" -eq 0 ] || fail "compositor exit status $COMPOSITOR_STATUS"

# Screenshot file exists
[ -f "$SCE" ] || fail "screenshot file not created"

# The client buffer is 200×100 at origin (0,0).
# Client rendered green with EGL PBuffer (0, 255, 0) in RGBA format.
# With --egl the compositor uploads RGBA directly (no swizzle).
# PPM Y-flip means rows 0-99 are the buffer area, cols 0-199.

# Green at (100, 50) — inside the buffer
python3 "$(dirname "$0")/ppm_pixel.py" "$SCE" 100 50 00ff00 40 || fail "pixel (100,50) should be green"

# Clear color at (600, 500) — outside buffer
python3 "$(dirname "$0")/ppm_pixel.py" "$SCE" 600 500 262633 20 || fail "pixel (600,500) should be clear color"

echo "PASS render-egl"
rm -f "$SCE"
