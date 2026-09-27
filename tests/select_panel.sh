#!/bin/sh
# P3-T05: select-panel — in World mode a ray from the screen centre
# targets the panel; pressing Enter (key 28) enters Application mode.
. "$(dirname "$0")/lib.sh"

SCRIPT="$WORK/input-script.txt"

# Build the input script:
#   1. Wait for clients to connect.
#   2. Verify World mode: key 10 NOT received.
#   3. Press Enter (key 28) to switch to Application mode.
#   4. Verify Application mode: key 43 IS received.
#   5. Quit.
cat > "$SCRIPT" <<EOF
wait 1000
key 10 press
key 10 release
key 28 press
key 43 press
key 43 release
quit
EOF

start_compositor --exit-after-ms 10000 --input-script "$SCRIPT"
run_client --toplevel --buffer 200x100 --color 00ff00 --report-input --exit-after-ms 5000

expect_line "$WORK/client.out" "^connected$"

# In World mode (before Enter), key events should NOT reach the client.
if grep -q "^key 10 " "$WORK/client.out"; then
    fail "key 10 was received by client in World mode (should not)"
fi

# After Enter (key 28), we are in Application mode.
expect_line "$WORK/client.out" "^key 43 1$"
expect_line "$WORK/client.out" "^key 43 0$"

expect_line "$WORK/client.out" "^done$"

wait_compositor
[ "$COMPOSITOR_STATUS" -eq 0 ] || fail "compositor exit status $COMPOSITOR_STATUS"

echo "PASS select-panel"
