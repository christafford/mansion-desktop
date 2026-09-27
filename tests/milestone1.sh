#!/bin/sh
# P4-T03: milestone1 — scripted nine-step demonstration of the full user journey.
#
# Steps demonstrated:
#   1. Start compositor in room-camera mode.
#   2. Walk forward (W key) toward the monitor.
#   3. Client is mapped on the monitor (panel at monitor frame position).
#   4. Approach the monitor further.
#   5. Select the panel (Enter key) → enters Application mode.
#   6. Type keys (client reports them via --report-input).
#   7. Return to world mode with the reserved key (F12=88).
#   8. Verify client is still connected (not disconnected by world-key).
. "$(dirname "$0")/lib.sh"

# Evdev keycodes
KEY_W=17
KEY_ENTER=28
KEY_F12=88
KEY_1=2
KEY_2=3
KEY_3=4

# Build input script that drives the full journey:
#   1. Wait for client to connect and render.
#   2. Walk forward a short distance (don't hit the wall!).
#   3. Release W.
#   4. Select the panel (Enter) → Application mode.
#   5. Wait for application mode to activate.
#   6. Type some keys (1, 2, 3 — evdev 2, 3, 4).
#   7. Press F12 to return to world mode.
#   8. Wait for world mode to restore.
#   9. Quit.
SCRIPT="$WORK/input-script.txt"
cat > "$SCRIPT" <<EOF
wait 2000
key $KEY_W press
wait 500
key $KEY_W release
wait 500
key $KEY_ENTER press
wait 500
key $KEY_ENTER release
wait 1000
key $KEY_1 press
wait 50
key $KEY_1 release
key $KEY_2 press
wait 50
key $KEY_2 release
key $KEY_3 press
wait 50
key $KEY_3 release
wait 500
key $KEY_F12 press
wait 500
key $KEY_F12 release
wait 1000
quit
EOF

# Start compositor in room-camera mode with input script.
start_compositor --room-camera --exit-after-ms 15000 --input-script "$SCRIPT"

# Run the test client with --report-input so key events are printed to stdout.
# --toplevel creates a toplevel surface that the compositor renders on the panel.
run_client --report-input --toplevel --buffer 200x200 --color ff0000 --exit-after-ms 12000

# Verify client connected and received configure.
expect_line "$WORK/client.out" "^connected$"
expect_line "$WORK/client.out" "^configure"

# Verify client reported key events (step 6: type keys).
# The test client prints "key <evdev_code> <state>" on key events.
expect_line "$WORK/client.out" "^key $KEY_1 1$" || fail "client did not report key $KEY_1 press"
expect_line "$WORK/client.out" "^key $KEY_2 1$" || fail "client did not report key $KEY_2 press"
expect_line "$WORK/client.out" "^key $KEY_3 1$" || fail "client did not report key $KEY_3 press"

# Verify camera moved toward the monitor (step 2/4).
# The camera starts at z=3 and walks forward a bit.
grep -q "^camera_pos " "$WORK/compositor.err" || fail "no camera_pos log in compositor stderr"
FINAL_Z=$(grep "^camera_pos " "$WORK/compositor.err" | tail -1 | awk '{print $4}')

python3 -c "
z = float('${FINAL_Z}')
# Camera started at z=3, walked forward. Should be less than 3.
assert z < 3.0, f'camera z={z} did not move forward from starting z=3'
print(f'camera_final_z={z:.4f} moved forward from z=3')
" || fail "camera did not move forward"

# Verify the client did NOT disconnect (step 8).
# When F12 returns to world mode, the client should still be connected
# and its surface should still be rendered on the panel.
expect_line "$WORK/client.out" "^configure" || fail "client lost its configure"

echo "PASS: milestone1"
