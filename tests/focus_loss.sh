#!/bin/sh
# P3-T03: focus-loss — verify that host focus loss saves mode and focus regain
# re-enters the client only if mode is still Application.
. "$(dirname "$0")/lib.sh"

SCRIPT="$WORK/input-script.txt"

# Build the input script:
#   1. Wait for clients to connect.
#   2. Switch to Application mode, grab focus.
#   3. Press key 42 (client should see it).
#   4. Focus lost — should exit app mode (key released, leave sent).
#   5. Focus gained — should re-enter app mode (key should be received again).
#   6. Press key 42 again (client should see it).
#   7. Quit.
cat > "$SCRIPT" <<EOF
wait 1000
mode app
focus gained
key 42 press
focus lost
focus gained
key 42 press
quit
EOF

start_compositor --exit-after-ms 10000 --input-script "$SCRIPT"
run_client --toplevel --buffer 200x100 --color 00ff00 --report-input --exit-after-ms 5000

expect_line "$WORK/client.out" "^connected$"
expect_line "$WORK/client.out" "^kbd_enter$"

# First key 42 press in Application mode.
expect_line "$WORK/client.out" "^key 42 1$"

# Focus lost → key 42 release + kbd_leave.
expect_line "$WORK/client.out" "^key 42 0$"
expect_line "$WORK/client.out" "^kbd_leave$"

# Focus gained → re-enter Application mode, kbd_enter again.
expect_line "$WORK/client.out" "^kbd_enter$"

# Second key 42 press should be received (re-entered Application mode).
expect_line "$WORK/client.out" "^key 42 1$"

expect_line "$WORK/client.out" "^done$"

wait_compositor
[ "$COMPOSITOR_STATUS" -eq 0 ] || fail "compositor exit status $COMPOSITOR_STATUS"

echo "PASS focus-loss"
