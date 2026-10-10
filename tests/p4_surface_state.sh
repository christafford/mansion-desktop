#!/bin/sh
# P4-T09: pending/current surface state edge cases.
# Tests: attach-without-commit, commit-without-attach, replaced pending,
#        null attach/commit, repeated buffer reuse after release,
#        resource destruction after release.

. "$(dirname "$0")/lib.sh"

run_test() {
    _name="$1"
    _client_args="$2"
    _expect_lines="$3"

    SOCKET="SOCKET-$_name"
    COMPOSITOR_PID=""

    "$ELSEWHERE" --headless --socket "$SOCKET" \
        --camera 0,0,10,0,0 \
        --exit-after-ms 3000 \
        >"$WORK/compositor.out" 2>"$WORK/compositor.err" &
    COMPOSITOR_PID=$!

    # Wait for socket
    for _ in $(seq 1 100); do
        if grep -q "^ELSEWHERE_SOCKET=$SOCKET\$" "$WORK/compositor.out" 2>/dev/null; then
            break
        fi
        if ! kill -0 "$COMPOSITOR_PID" 2>/dev/null; then
            fail "$_name: compositor exited early"
        fi
        sleep 0.05
    done

    run_client --toplevel --buffer 200x200 $_client_args \
        >"$WORK/client.out" 2>"$WORK/client.err" || true

    wait_compositor 2>/dev/null || true

    # Check expected output lines
    for _expected in $_expect_lines; do
        if ! expect_line "$WORK/client.out" "$_expected"; then
            fail "$_name: missing expected line '$_expected'"
        fi
    done

    echo "PASS $_name"
}

# ── Test 1: attach-without-commit ──────────────────────────────────
# Client attaches a buffer but never commits it. The surface should
# show nothing (no buffer was committed).
run_test "attach-only" "--color 00ff00 --attach-only" \
    "^connected$"

# ── Test 2: commit without attach ──────────────────────────────────
# Client commits without a new attach. The first commit creates
# an empty surface; second commit without new attach should still
# succeed and fire another frame callback.
run_test "commit-only" "--color ff0000 --commit-only" \
    "^connected$ ^frame [0-9]+$ ^frame [0-9]+$"

# ── Test 3: replaced pending attach ────────────────────────────────
# Client attaches buffer A, then attaches buffer B (without committing A),
# then commits. Only B should be rendered.
SCREEN_P3="$(mktemp /tmp/screenshot_p3.XXXXXX.ppm)"
SOCKET="SOCKET-replaced-attach"
COMPOSITOR_PID=""

"$ELSEWHERE" --headless --socket "$SOCKET" \
    --camera 0,0,10,0,0 \
    --exit-after-ms 2000 \
    --screenshot "$SCREEN_P3" \
    >"$WORK/compositor.out" 2>"$WORK/compositor.err" &
COMPOSITOR_PID=$!

for _ in $(seq 1 100); do
    if grep -q "^ELSEWHERE_SOCKET=$SOCKET\$" "$WORK/compositor.out" 2>/dev/null; then
        break
    fi
    if ! kill -0 "$COMPOSITOR_PID" 2>/dev/null; then
        fail "replaced-attach: compositor exited early"
    fi
    sleep 0.05
done

run_client --toplevel --buffer 200x200 --color ff0000 --commit-color 0000ff \
    --exit-after-ms 500 \
    >"$WORK/client.out" 2>"$WORK/client.err" || true

# Wait for client to finish (it will exit after 500ms, giving compositor
# time to dispatch and render the blue buffer).
sleep 0.3

wait_compositor 2>/dev/null || true

# Panel centre (512, 384) should be blue (second/last commit wins).
python3 "$(dirname "$0")/ppm_pixel.py" "$SCREEN_P3" 512 384 0000ff 40 || \
    fail "replaced-attach: pixel (512,384) should be blue"
echo "PASS replaced-attach"
rm -f "$SCREEN_P3"

# ── Test 4: null attach/commit ─────────────────────────────────────
# Client attaches null buffer and commits. The current buffer should
# be cleared (no buffer to render).
run_test "null-attach" "--color ff0000 --null-attach" \
    "^connected$ ^frame [0-9]+$ ^frame [0-9]+$"

# ── Test 5: repeated buffer reuse after release ────────────────────
# Client commits buffer A, receives release, commits buffer B.
# The --commit-color flag tests this: first commit red, then blue.
# Both frame callbacks and both releases should arrive.
run_test "buffer-reuse" "--color ff0000 --commit-color 00ff00 --exit-after-ms 500" \
    "^connected$ ^frame [0-9]+$ ^release$ ^frame [0-9]+$ ^release$"

# ── Test 6: resource destruction after release ─────────────────────
# Client receives buffer release, then destroys the buffer proxy.
# This tests that the compositor handles the buffer being destroyed
# after it's been released (no use-after-free).
run_test "destroy-after-release" "--color ff0000 --destroy-after-release" \
    "^connected$ ^frame [0-9]+$ ^release$ ^buffer_destroyed$"

echo "PASS p4_surface_state"
