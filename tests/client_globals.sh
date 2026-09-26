#!/bin/sh
# P1-T02: the test client connects and sees the core globals.
. "$(dirname "$0")/lib.sh"

start_compositor --exit-after-ms 5000
run_client --exit-after-ms 100

expect_line "$WORK/client.out" "^connected$"
expect_line "$WORK/client.out" "^global wl_compositor [0-9]+$"
expect_line "$WORK/client.out" "^global wl_shm [0-9]+$"
expect_line "$WORK/client.out" "^global wl_seat [0-9]+$"
expect_line "$WORK/client.out" "^done$"

# The compositor must survive a client connecting and disconnecting.
kill -0 "$COMPOSITOR_PID" || fail "compositor died after the client disconnected"
kill -TERM "$COMPOSITOR_PID"
wait_compositor
[ "$COMPOSITOR_STATUS" -eq 0 ] || fail "compositor exit status $COMPOSITOR_STATUS after SIGTERM"

echo "PASS client-globals"
