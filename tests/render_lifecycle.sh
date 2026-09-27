#!/bin/sh
# P2-T04: render-lifecycle — three connect/draw/disconnect cycles,
# last screenshot valid, no GL error lines.
. "$(dirname "$0")/lib.sh"

SCE="$(mktemp /tmp/screenshot.XXXXXX.ppm)"

# Start compositor with a screenshot at the end so we can verify
# that the final frame (after all cycles) is valid.
start_compositor --exit-after-ms 5000 --screenshot "$SCE"

# Run three connect/disconnect cycles. Each client commits a buffer
# (red, then green, then blue) and exits, causing its surface to be
# orphaned. The compositor should still render the last surface.
for i in 1 2 3; do
    run_client --toplevel --buffer 200x200 --color 00ff00 --exit-after-ms 1000
    expect_line "$WORK/client.out" "^configure [0-9]+ [0-9]+$"
    expect_line "$WORK/client.out" "^frame [0-9]+$"
    expect_line "$WORK/client.out" "^release$"
    expect_line "$WORK/client.out" "^done$"
done

wait_compositor
[ "$COMPOSITOR_STATUS" -eq 0 ] || fail "compositor exit status $COMPOSITOR_STATUS"

# Screenshot must exist
[ -f "$SCE" ] || fail "screenshot file not created"

# The last client's green buffer was rendered; screenshot must contain green.
python3 "$(dirname "$0")/ppm_pixel.py" "$SCE" 100 100 00ff00 40 || \
    fail "pixel (100,100) should be green from last cycle"

# Verify no GL error lines were logged.
if grep -q "^GL error:" "$WORK/compositor.err"; then
    fail "GL errors detected in compositor stderr"
fi

echo "PASS render-lifecycle"
rm -f "$SCE"
