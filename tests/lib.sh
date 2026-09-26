# Shared helpers for Mansion Desktop shell tests. Source this file; do not execute it.
#
# Every test gets a private XDG_RUNTIME_DIR so it never touches the host session's
# sockets, and a private socket name so tests can run in parallel.

set -u

MANSION="${1:?usage: test.sh <mansion-desktop binary> [test client binary]}"
CLIENT="${2:-}"

WORK="$(mktemp -d "${TMPDIR:-/tmp}/mansion-test.XXXXXX")"
export XDG_RUNTIME_DIR="$WORK/runtime"
mkdir -m 700 "$XDG_RUNTIME_DIR"
unset WAYLAND_DISPLAY DISPLAY
SOCKET="mansion-test-$$"
COMPOSITOR_PID=""

fail() {
    echo "FAIL: $*" >&2
    [ -f "$WORK/compositor.err" ] && { echo "--- compositor stderr ---" >&2; cat "$WORK/compositor.err" >&2; }
    exit 1
}

cleanup() {
    if [ -n "$COMPOSITOR_PID" ] && kill -0 "$COMPOSITOR_PID" 2>/dev/null; then
        kill "$COMPOSITOR_PID" 2>/dev/null
        wait "$COMPOSITOR_PID" 2>/dev/null
    fi
    rm -rf "$WORK"
}
trap cleanup EXIT

# start_compositor [extra args...]  -> background headless compositor; waits until the socket is listening.
start_compositor() {
    "$MANSION" --headless --socket "$SOCKET" "$@" >"$WORK/compositor.out" 2>"$WORK/compositor.err" &
    COMPOSITOR_PID=$!
    for _ in $(seq 1 100); do
        if grep -q "^MANSION_SOCKET=$SOCKET\$" "$WORK/compositor.out" 2>/dev/null; then
            return 0
        fi
        if ! kill -0 "$COMPOSITOR_PID" 2>/dev/null; then
            fail "compositor exited early"
        fi
        sleep 0.05
    done
    fail "compositor did not announce its socket"
}

# wait_compositor -> waits for the background compositor and stores its status in COMPOSITOR_STATUS.
wait_compositor() {
    wait "$COMPOSITOR_PID"
    COMPOSITOR_STATUS=$?
    COMPOSITOR_PID=""
}

# run_client [args...] -> runs the test client against the compositor; stdout to $WORK/client.out.
run_client() {
    [ -n "$CLIENT" ] || fail "no test client binary given"
    "$CLIENT" --socket "$SOCKET" "$@" >"$WORK/client.out" 2>"$WORK/client.err" || {
        echo "--- client stderr ---" >&2; cat "$WORK/client.err" >&2
        fail "test client exited with status $?"
    }
}

expect_line() { # expect_line FILE REGEX
    grep -Eq "$2" "$1" || { echo "--- $1 ---" >&2; cat "$1" >&2; fail "expected /$2/ in $1"; }
}
