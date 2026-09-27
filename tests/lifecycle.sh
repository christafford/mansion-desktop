#!/bin/sh
# P1-T09: lifecycle robustness.
# Three connect/disconnect cycles, then a fresh client still gets configure.
. "$(dirname "$0")/lib.sh"

start_compositor --exit-after-ms 5000

# Run three connect/disconnect cycles, then a fourth client to verify
# the compositor is still accepting connections.
for i in 1 2 3 4; do
    run_client --toplevel
    expect_line "$WORK/client.out" "^configure [0-9]+ [0-9]+$"
done

wait_compositor
[ "$COMPOSITOR_STATUS" -eq 0 ] || fail "compositor exit status $COMPOSITOR_STATUS"

echo "PASS lifecycle"
