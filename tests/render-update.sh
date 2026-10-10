#!/bin/sh
# P2-T03: render-update — live updates via damage tracking.
# Client commits red, compositor renders red; client commits blue,
# compositor renders blue at the projected panel centre.
#
# This test runs two compositor/client pairs to prove that
# textures are re-uploaded only when a surface is committed since
# the last frame (P2-T03 damage tracking).
#
# Camera:   (0, 0, 10) looking at origin (panel at 0,0,0)
# Viewport: 1024 × 768
# Projected panel centre: (512, 384)

. "$(dirname "$0")/lib.sh"

run_pair() {
    _socket="$1"
    _screenshot="$2"
    _compositor_args="$3"
    _client_args="$4"
    _expect_lines="$5"

    # Override socket for this run; reset PID so cleanup doesn't kill it.
    SOCKET="$_socket"
    COMPOSITOR_PID=""

    # Start compositor.
    "$ELSEWHERE" --headless --socket "$SOCKET" \
        --camera 0,0,10,0,0 \
        --exit-after-ms "$_compositor_args" \
        --screenshot "$_screenshot" \
        >"$WORK/compositor.out" 2>"$WORK/compositor.err" &
    COMPOSITOR_PID=$!

    # Wait for compositor socket to appear.
    for _ in $(seq 1 100); do
        if grep -q "^ELSEWHERE_SOCKET=$_socket\$" "$WORK/compositor.out" 2>/dev/null; then
            break
        fi
        if ! kill -0 "$COMPOSITOR_PID" 2>/dev/null; then
            fail "$_socket: compositor exited early"
        fi
        sleep 0.05
    done

    # Run client.
    run_client --socket "$SOCKET" --toplevel --buffer 200x200 $_client_args
    # Verify client output.
    _line=0
    for _expected in $_expect_lines; do
        _line=$((_line + 1))
        expect_line "$WORK/client.out" "$_expected"
    done
    wait_compositor
    [ "$COMPOSITOR_STATUS" -eq 0 ] || fail "$_socket: compositor exit status $COMPOSITOR_STATUS"
    [ -f "$_screenshot" ] || fail "$_socket: screenshot not created"
}

# ── Run A: red buffer only ───────────────────────────────────────────
SCREENA="$(mktemp /tmp/screenshotA.XXXXXX.ppm)"
run_pair "$SOCKET-A" "$SCREENA" 1500 \
    "--color ff0000 --exit-after-ms 1000" \
    "^connected$ ^frame [0-9]+$"

# Panel centre (512, 384) should be red.
python3 "$(dirname "$0")/ppm_pixel.py" "$SCREENA" 512 384 ff0000 40 || \
    fail "Run A: pixel (512,384) should be red"

# ── Run B: red → blue update ─────────────────────────────────────────
SCREENB="$(mktemp /tmp/screenshotB.XXXXXX.ppm)"
run_pair "$SOCKET-B" "$SCREENB" 2500 \
    "--color ff0000 --commit-color 0000ff --exit-after-ms 2000" \
    "^connected$ ^frame [0-9]+$ ^frame [0-9]+$"

# Panel centre (512, 384) should be blue (second commit).
python3 "$(dirname "$0")/ppm_pixel.py" "$SCREENB" 512 384 0000ff 40 || \
    fail "Run B: pixel (512,384) should be blue"

echo "PASS render-update"
rm -f "$SCREENA" "$SCREENB"
