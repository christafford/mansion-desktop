#!/bin/sh
# P3-T02: mode-exit — verify that pressing the world-key (default F12=88)
# cleanly exits Application mode: held keys released, leave events sent.
. "$(dirname "$0")/lib.sh"

SCRIPT="$WORK/input-script.txt"

# Build the input script:
#   1. Wait for clients to connect.
#   2. Switch to Application mode, grab keyboard focus.
#   3. Press a held key (42).
#   4. Press world key (88=F12) — should exit to World mode.
#   5. Quit.
cat > "$SCRIPT" <<EOF
wait 1000
mode app
focus gained
key 42 press
key 88 press
quit
EOF

start_compositor --exit-after-ms 10000 --input-script "$SCRIPT"
run_client --toplevel --buffer 200x100 --color 00ff00 --report-input --exit-after-ms 5000

expect_line "$WORK/client.out" "^connected$"
expect_line "$WORK/client.out" "^kbd_enter$"

# In Application mode, key 42 press is received.
expect_line "$WORK/client.out" "^key 42 1$"

# World key (88) press should trigger exit_application_mode(),
# which sends a release for the held key (42).
expect_line "$WORK/client.out" "^key 42 0$"

# After exiting Application mode, keyboard leave should be sent.
expect_line "$WORK/client.out" "^kbd_leave$"

expect_line "$WORK/client.out" "^done$"

wait_compositor
[ "$COMPOSITOR_STATUS" -eq 0 ] || fail "compositor exit status $COMPOSITOR_STATUS"

echo "PASS mode-exit"
