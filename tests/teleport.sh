#!/bin/sh
# P4-T02: teleport — key T (evdev 20) teleports camera to stored viewpoint.
#
# 1. Start compositor in room-camera mode.
# 2. Press 'w' (key 17) to move the camera forward.
# 3. Press T (key 20) to teleport back to the stored viewpoint.
# 4. The final camera position must equal the teleport target (0, 3, 0).
. "$(dirname "$0")/lib.sh"

SCRIPT="$WORK/input-script.txt"
cat > "$SCRIPT" <<EOF
wait 1500
key 17 press
wait 1000
key 17 release
key 20 press
wait 1000
quit
EOF

start_compositor --room-camera --exit-after-ms 10000 --input-script "$SCRIPT"
run_client --toplevel --buffer 200x100 --color ff0000 --exit-after-ms 5000

expect_line "$WORK/client.out" "^connected$"
expect_line "$WORK/client.out" "^configure"

wait_compositor
[ "$COMPOSITOR_STATUS" -eq 0 ] || fail "compositor exit status $COMPOSITOR_STATUS"

# Verify teleport target was reached.
# The teleport target is (x=0, y=3, z=0).
grep -q "^camera_pos " "$WORK/compositor.err" || fail "no camera_pos log in compositor stderr"

python3 -c "
import sys
lines = [l for l in open('$WORK/compositor.err') if l.startswith('camera_pos ')]
if not lines:
    sys.exit(1)
# The last camera_pos line after teleport should be the target.
parts = lines[-1].strip().split()
x, y, z = float(parts[1]), float(parts[2]), float(parts[3])
tolerance = 0.5
ok = abs(x - 0.0) < tolerance and abs(y - 3.0) < tolerance and abs(z - 0.0) < tolerance
print(f'camera_pos=({x:.4f}, {y:.4f}, {z:.4f}) teleport_target=(0, 3, 0)')
if not ok:
    sys.exit(1)
" || fail "camera not at teleport target (0, 3, 0)"

echo "PASS: teleport"
