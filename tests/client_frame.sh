#!/bin/sh
# P1-T04: the test client creates a shm buffer, attaches it, requests a
# frame callback, and receives frame <time> + release within 2 seconds.
. "$(dirname "$0")/lib.sh"

start_compositor --exit-after-ms 5000
run_client --toplevel --buffer 200x100 --color ff0000 --exit-after-ms 2000

expect_line "$WORK/client.out" "^connected$"
expect_line "$WORK/client.out" "^configure 800 600$"
expect_line "$WORK/client.out" "^frame [0-9]+$"
expect_line "$WORK/client.out" "^release$"
expect_line "$WORK/client.out" "^done$"

# The compositor must survive the client connecting and disconnecting.
kill -0 "$COMPOSITOR_PID" || fail "compositor died after the client disconnected"
kill -TERM "$COMPOSITOR_PID"
wait_compositor
[ "$COMPOSITOR_STATUS" -eq 0 ] || fail "compositor exit status $COMPOSITOR_STATUS after SIGTERM"

echo "PASS client-frame"
