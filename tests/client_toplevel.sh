#!/bin/sh
# P1-T03: the test client connects, creates a toplevel, receives
# an 800x600 configure line, and the compositor survives.
. "$(dirname "$0")/lib.sh"

start_compositor --exit-after-ms 5000
run_client --toplevel --exit-after-ms 1000

expect_line "$WORK/client.out" "^connected$"
expect_line "$WORK/client.out" "^configure 800 600$"
expect_line "$WORK/client.out" "^done$"

# The compositor must survive a client connecting and disconnecting.
kill -0 "$COMPOSITOR_PID" || fail "compositor died after the client disconnected"
kill -TERM "$COMPOSITOR_PID"
wait_compositor
[ "$COMPOSITOR_STATUS" -eq 0 ] || fail "compositor exit status $COMPOSITOR_STATUS after SIGTERM"

echo "PASS client-toplevel"
