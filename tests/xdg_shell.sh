#!/bin/sh
# P4-T11: xdg-shell handshake and metadata validation.
# Tests: version advertisement, configure/ack/commit ordering,
# metadata (title/app_id/geometry), repeated connect/disconnect.
. "$(dirname "$0")/lib.sh"

start_compositor --exit-after-ms 5000

# --- Test 1: version advertisement and initial configure ---
run_client --toplevel --exit-after-ms 1000
expect_line "$WORK/client.out" "^global xdg_wm_base 3$"
expect_line "$WORK/client.out" "^configure 800 600$"
expect_line "$WORK/client.out" "^done$"
kill -0 "$COMPOSITOR_PID" || fail "compositor died"
kill -TERM "$COMPOSITOR_PID"
wait_compositor
[ "$COMPOSITOR_STATUS" -eq 0 ] || fail "compositor exit status $COMPOSITOR_STATUS after test 1"

# --- Test 2: xdg-test mode (metadata: title, app_id, geometry) ---
start_compositor --exit-after-ms 5000
run_client --xdg-test --exit-after-ms 2000
expect_line "$WORK/client.out" "^xdg-metadata-set$"
expect_line "$WORK/client.out" "^xdg-configure-acked$"
expect_line "$WORK/client.out" "^xdg-test-done$"
kill -0 "$COMPOSITOR_PID" || fail "compositor died during xdg-test"
kill -TERM "$COMPOSITOR_PID"
wait_compositor
[ "$COMPOSITOR_STATUS" -eq 0 ] || fail "compositor exit status $COMPOSITOR_STATUS after test 2"

# --- Test 3: repeated connect/disconnect survives ---
start_compositor --exit-after-ms 5000
for i in 1 2 3; do
    run_client --toplevel --exit-after-ms 1000
    expect_line "$WORK/client.out" "^configure 800 600$"
done
kill -0 "$COMPOSITOR_PID" || fail "compositor died after repeated clients"
kill -TERM "$COMPOSITOR_PID"
wait_compositor
[ "$COMPOSITOR_STATUS" -eq 0 ] || fail "compositor exit status $COMPOSITOR_STATUS after test 3"

echo "PASS xdg-shell"
