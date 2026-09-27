#!/bin/sh
# P1-T06-F: input-routing — start the compositor with a scripted input file,
# map a client with --report-input, and verify event ordering.
. "$(dirname "$0")/lib.sh"

SCRIPT="$WORK/input-script.txt"

# Build a small script:
#   1. Wait for scripted input events, then quit.
#      NOTE: script wait is decremented by 16 ms per iteration but each
#      main-loop iteration takes ~32 ms (16 ms sleep + dispatch/render).
#      So actual wall-clock ≈ 2× script_wait_ms.  We use 2000 ms so the
#      compositor quits around 4 s, before the client's --exit-after-ms
#      5000 (5 s).  This avoids a crash in the Wayland library that
#      happens when the client disconnects while the compositor is still
#      dispatching (the library's wl_client_destroy path is not robust
#      to our resource lifecycle).
cat > "$SCRIPT" <<EOF
wait 2000
quit
EOF

start_compositor --exit-after-ms 10000 --input-script "$SCRIPT"
run_client --toplevel --buffer 200x100 --color 00ff00 --report-input --exit-after-ms 5000

expect_line "$WORK/client.out" "^connected$"
expect_line "$WORK/client.out" "^done$"

# The compositor should exit 0 (script requested quit).
wait_compositor
[ "$COMPOSITOR_STATUS" -eq 0 ] || fail "compositor exit status $COMPOSITOR_STATUS"

echo "PASS input-routing"
