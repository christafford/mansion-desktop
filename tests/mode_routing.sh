#!/bin/sh
# P3-T01: mode-routing — verify that input is routed based on InputMode.
# In World mode: key/motion/button events are consumed (no client input).
# In Application mode: events go to the focused surface.
. "$(dirname "$0")/lib.sh"

SCRIPT="$WORK/input-script.txt"

# Build the input script:
#   1. Wait for clients to connect.
#   2. Switch to World mode, send a key event (client should NOT see it).
#   3. Switch to Application mode, send a key event (client SHOULD see it).
#   4. Quit.
cat > "$SCRIPT" <<EOF
wait 1000
mode app
focus gained
key 10 press
key 10 release
mode world
key 42 press
key 42 release
mode app
key 43 press
key 43 release
quit
EOF

start_compositor --exit-after-ms 10000 --input-script "$SCRIPT"
run_client --toplevel --buffer 200x100 --color 00ff00 --report-input --exit-after-ms 5000

expect_line "$WORK/client.out" "^connected$"

# In Application mode (before mode world), the client receives key events.
expect_line "$WORK/client.out" "^key 10 1$"
expect_line "$WORK/client.out" "^key 10 0$"

# In World mode, key events should NOT reach the client.
# We verify this by checking that key 42 does NOT appear.
if grep -q "^key 42 " "$WORK/client.out"; then
    fail "key 42 was received by client in World mode (should not)"
fi

# In Application mode again, key events should reach the client.
expect_line "$WORK/client.out" "^key 43 1$"
expect_line "$WORK/client.out" "^key 43 0$"

expect_line "$WORK/client.out" "^done$"

wait_compositor
[ "$COMPOSITOR_STATUS" -eq 0 ] || fail "compositor exit status $COMPOSITOR_STATUS"

echo "PASS mode-routing"
