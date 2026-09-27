#!/bin/sh
# P1-T08: launcher child lifecycle.
# The compositor launches a test client via --launch, logs client connect/disconnect
# and child exit, then exits 0 itself.
. "$(dirname "$0")/lib.sh"

start_compositor --launch "$CLIENT --toplevel --buffer 64x64 --exit-after-ms 300" \
                --exit-after-ms 3000

# The client should connect and disconnect; compositor should exit on timeout.
wait_compositor
[ "$COMPOSITOR_STATUS" -eq 0 ] || fail "compositor exit status $COMPOSITOR_STATUS"

# Check stderr for expected log messages.
err="$WORK/compositor.err"
[ -f "$err" ] || fail "no compositor stderr"

expect_line "$err" "client connected [0-9]"
expect_line "$err" "client disconnected [0-9]"
expect_line "$err" "child [0-9]+ exited [0-9]+"

echo "PASS launch-client"
