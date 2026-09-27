#!/bin/sh
# P2-T02: render-panel — with a fixed camera the client colour is present
# at the projected panel centre and the clear colour at a pixel outside
# the projected quad.
#
# Camera:   (0, 0, 10) looking at origin (panel at 0,0,0)
# Viewport: 1024 × 768
# Projected panel centre: (512, 384)
# Projected panel quad:  ~(474,355) to ~(550,412)

. "$(dirname "$0")/lib.sh"

SCE="$(mktemp /tmp/screenshot.XXXXXX.ppm)"

# Start compositor in 3D panel mode with fixed camera.
# Camera: X=0, Y=0, Z=10, yaw=0, pitch=0
start_compositor --camera 0,0,10,0,0 --exit-after-ms 3000 --screenshot "$SCE"

# Client draws a red buffer (200×200) — it will be rendered onto the panel.
run_client --toplevel --buffer 200x200 --color ff0000 --exit-after-ms 2000

# Client lifecycle
expect_line "$WORK/client.out" "^connected$"
expect_line "$WORK/client.out" "^configure 800 600$"
expect_line "$WORK/client.out" "^frame [0-9]+$"
expect_line "$WORK/client.out" "^release$"
expect_line "$WORK/client.out" "^done$"

wait_compositor
[ "$COMPOSITOR_STATUS" -eq 0 ] || fail "compositor exit status $COMPOSITOR_STATUS"

[ -f "$SCE" ] || fail "screenshot file not created"

# PPM pixel at panel centre (512, 384) should be red (the client colour).
# Tolerance 40 accounts for texture sampling / mip-mapping edge effects.
python3 "$(dirname "$0")/ppm_pixel.py" "$SCE" 512 384 ff0000 40 || \
    fail "pixel (512,384) at panel centre should be red"

# Pixel outside the panel quad should be the clear colour.
# Clear colour is (0.15, 0.15, 0.2) ≈ (38, 38, 51) → hex 262633
python3 "$(dirname "$0")/ppm_pixel.py" "$SCE" 600 500 262633 20 || \
    fail "pixel (600,500) outside panel should be clear colour"

echo "PASS render-panel"
rm -f "$SCE"
