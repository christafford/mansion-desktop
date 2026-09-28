#!/bin/sh
# P5-T02: focus-cycle — verify that tab/shift_tab commands cycle keyboard
# focus through the window registry without crashing.
#
# Flow:
#   1. Start compositor with two clients connected concurrently.
#   2. Input script does multiple tab/shift_tab cycles in World mode.
#   3. Both clients connect and disconnect.
#   4. Compositor must exit cleanly (proves no crash from focus cycling).
. "$(dirname "$0")/lib.sh"

SCRIPT="$WORK/input-script.txt"

cat > "$SCRIPT" <<EOF
wait 2000
tab
wait 200
tab
wait 200
shift_tab
wait 200
tab
wait 200
shift_tab
quit
EOF

start_compositor --exit-after-ms 10000 --input-script "$SCRIPT"

# First client.
run_client --toplevel --buffer 200x100 --color 00ff00 --exit-after-ms 5000
expect_line "$WORK/client.out" "^connected$"
expect_line "$WORK/client.out" "^configure 800 600$"

# Second client (overwrites client.out).
run_client --toplevel --buffer 100x100 --color ff0000 --exit-after-ms 5000
expect_line "$WORK/client.out" "^connected$"
expect_line "$WORK/client.out" "^configure 800 600$"

# Wait for clients to finish, then for compositor to exit.
sleep 1

wait_compositor
[ "$COMPOSITOR_STATUS" -eq 0 ] || fail "compositor exit status $COMPOSITOR_STATUS"

echo "PASS focus-cycle"
