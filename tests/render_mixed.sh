#!/bin/sh
# P4-T10: render-mixed — verify format-aware swizzle renders mixed RGB
# correctly (not just pure colours).  The test client writes a 100×100
# buffer with RGB=(127, 63, 31) which is not pure red/green/blue.
# Without the correct ARGB8888 → GL_RGBA swizzle this would render
# incorrectly (e.g. R↔B swapped).
. "$(dirname "$0")/lib.sh"

SCE="$(mktemp /tmp/screenshot.XXXXXX.ppm)"

# Client writes ARGB8888 data — hex colour 7f3f1f = R=127, G=63, B=31.
# Compositor must swizzle to GL_RGBA and render the same mixed colour.
start_compositor --exit-after-ms 3000 --screenshot "$SCE"
run_client --buffer 100x100 --color 7f3f1f --exit-after-ms 2000

# Client lifecycle
expect_line "$WORK/client.out" "^connected$"
expect_line "$WORK/client.out" "^configure 800 600$"
expect_line "$WORK/client.out" "^frame [0-9]+$"
expect_line "$WORK/client.out" "^release$"
expect_line "$WORK/client.out" "^done$"

wait_compositor
[ "$COMPOSITOR_STATUS" -eq 0 ] || fail "compositor exit status $COMPOSITOR_STATUS"

[ -f "$SCE" ] || fail "screenshot file not created"

# Mixed colour at (50, 50) — inside the 100×100 buffer at origin
# Allow generous tolerance because GL texture sampling blends neighbour pixels.
python3 "$(dirname "$0")/ppm_pixel.py" "$SCE" 50 50 7f3f1f 40 || fail "pixel (50,50) should be (127,63,31) mixed colour"

# Clear colour outside buffer
python3 "$(dirname "$0")/ppm_pixel.py" "$SCE" 700 500 262633 20 || fail "pixel (700,500) should be clear colour"

echo "PASS render-mixed"
rm -f "$SCE"
