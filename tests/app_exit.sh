#!/bin/sh
# P3-T06: app-exit — when the focused client exits during Application mode,
# the compositor returns to World mode, clears focus, and has no dangling
# pointers.
#
# Flow:
#   1. Compositor starts; input script switches to Application mode.
#   2. First client connects in Application mode, receives key events.
#   3. First client exits → surface_destroy_callback fires → compositor
#      detects focused surface was destroyed in Application mode → switches
#      to World mode and clears seat focus pointers.
#   4. Input script sends key 42 press AFTER the mode switch.
#   5. Second client connects in World mode, receives kbd_enter (always)
#      but NOT key 42 (World mode discards key events).
#
# Verification:
#   - Second client receives kbd_enter but not key 42 press
#     (proves compositor is in World mode when second client connects)
#   - No crash during surface destruction or compositor cleanup
#     (proves no dangling pointers)
#
# Note: no `quit` in the input script — quit causes immediate exit.
. "$(dirname "$0")/lib.sh"

SCRIPT="$WORK/input-script.txt"

# Build the input script:
#   1. Wait for first client to connect.
#   2. Switch to Application mode.
#   3. Send key 42 press (first client is focused in App mode).
#   4. Wait for first client to exit and compositor to process surface destroy.
#   5. Send key 42 press — any connected client is now in World mode.
#   6. Wait for second client to finish.
cat > "$SCRIPT" <<EOF
wait 2000
mode app
key 42 press
wait 4000
key 42 press
wait 1000
EOF

start_compositor --exit-after-ms 15000 --input-script "$SCRIPT"

# First client: connects, commits buffer, stays 4 s then exits.
run_client --toplevel --buffer 200x100 --color 00ff00 --exit-after-ms 4000

# Wait for first client to exit and compositor to process surface destroy.
sleep 1

# Second client: connects after first client exited.
# In World mode it receives kbd_enter but not key 42 press (sent at t≈9s).
run_client --toplevel --buffer 100x100 --color ff0000 --report-input --exit-after-ms 3000

expect_line "$WORK/client.out" "^connected$"
expect_line "$WORK/client.out" "^configure 800 600$"

# The second client receives kbd_enter (always, regardless of mode).
expect_line "$WORK/client.out" "^kbd_enter$"

# After the compositor switched to World mode, the second key 42 press
# (sent at ~9 s) must NOT have reached the second client.
grep -q "^key 42" "$WORK/client.out" && \
    fail "second client should not receive key events in World mode"

# Wait for the compositor to exit naturally — if there are dangling
# pointers, the compositor will crash here during cleanup.
wait_compositor
[ "$COMPOSITOR_STATUS" -eq 0 ] || fail "compositor exit status $COMPOSITOR_STATUS"

echo "PASS app-exit"
