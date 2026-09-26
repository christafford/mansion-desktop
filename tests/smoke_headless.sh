#!/bin/sh
# P1-T01: headless compositor starts, listens on its private socket, exits 0 on its own,
# and removes the socket and lock file.
. "$(dirname "$0")/lib.sh"

start_compositor --exit-after-ms 300

[ -S "$XDG_RUNTIME_DIR/$SOCKET" ] || fail "socket $XDG_RUNTIME_DIR/$SOCKET missing while running"
[ -e "$XDG_RUNTIME_DIR/$SOCKET.lock" ] || fail "lock file missing while running"

wait_compositor
[ "$COMPOSITOR_STATUS" -eq 0 ] || fail "compositor exit status $COMPOSITOR_STATUS"
[ ! -e "$XDG_RUNTIME_DIR/$SOCKET" ] || fail "socket not removed on exit"
[ ! -e "$XDG_RUNTIME_DIR/$SOCKET.lock" ] || fail "lock file not removed on exit"
grep -q "input device" "$WORK/compositor.err" && fail "headless mode must not open input devices"

echo "PASS smoke-headless"
