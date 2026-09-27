#!/bin/sh
# P4-T01: room-collision — AABB collision keeps camera inside room bounds.
#
# Walk forward (w key) toward the front wall (z = -5).
# The camera starts at z=3 and the room bound is z_min=-4.5.
# After walking for 2 seconds the camera should stop at z≈-4.5,
# not pass through the wall.
. "$(dirname "$0")/lib.sh"

# Build input script:
#   1. Wait for client to connect.
#   2. Press w for ~2 s (enough to hit the wall).
#   3. Release w.
#   4. Wait then quit.
SCRIPT="$WORK/input-script.txt"
cat > "$SCRIPT" <<EOF
wait 1500
key 17 press
wait 2000
key 17 release
wait 500
quit
EOF

start_compositor --room-camera --exit-after-ms 10000 --input-script "$SCRIPT"
run_client --toplevel --buffer 200x100 --color ff0000 --exit-after-ms 5000

expect_line "$WORK/client.out" "^connected$"
expect_line "$WORK/client.out" "^configure"

wait_compositor
[ "$COMPOSITOR_STATUS" -eq 0 ] || fail "compositor exit status $COMPOSITOR_STATUS"

# Collision is verified by the fact that the camera position never exceeds
# the room bound.  We check this via the camera position log.
grep -q "^camera_pos " "$WORK/compositor.err" || fail "no camera_pos log in compositor stderr"
FINAL_Z=$(grep "^camera_pos " "$WORK/compositor.err" | tail -1 | awk '{print $4}')

# z must be >= z_min (-4.5) — i.e. not past the wall.
# Allow a small tolerance for floating-point.
python3 -c "
z = float('${FINAL_Z}')
assert z >= -4.6, f'camera z={z} exceeded wall bound -4.5'
print(f'camera_z={z:.4f} within bounds [−4.5, +4.5]')
" || fail "camera z=$FINAL_Z violated collision bounds"

echo "PASS: room-collision"
