# Godot compositor lifecycle — 2026-10-03

Historical T38 checkpoint (`5f7de10`). T39 is now complete; use
[handoff 18](18-godot-window-protocol.md) for current continuation and sanitizer commands.

P21-T38 connects the recovered native binding to the existing extracted
`compositor-core.cpp`. It serves real Wayland requests without linking the old
renderer. This is a protocol/lifetime gate, not a terminal or pixel-rendering gate.

## Ownership and behavior

`CompositorRuntime` owns the server display, wl_compositor/shm globals and one
private mode-0700 directory/socket. It accepts an absolute parent directory,
uses an absolute Wayland socket name, and never changes host environment variables.
`MansionCompositorSession` exposes start/pump/stop, socket path, error reporting
and counts to Godot. All calls and destruction belong to the owning main thread.

The pump calls `wl_event_loop_dispatch(server_loop, 0)` and flushes server
clients. Stop destroys clients before the core and display, then removes only
its own empty directory. Repeated stop and RefCounted destruction are covered.
A failed or double start returns an error rather than replacing a live server.

The existing partial core had concrete lifecycle defects. This change makes the
surface destroy request actually destroy its resource; unlinks buffer listeners
before freeing surfaces; gives frame callbacks a list-removal destructor; fixes
surface iteration to use MansionSurface links; initializes allocated state and
honors the client's bound protocol version. This core frees dead surface state
instead of keeping invalid Wayland resources in a renderer-style orphan cache.
The legacy compositor/renderer implementation is unchanged.

## Evidence

`tools/check-godot-binding.sh` now checks the original 300 native probe calls,
a standalone server and the same lifecycle through Godot. Each lifecycle runs
nine trials, rotating through:

1. Client explicitly destroys sixteen surfaces, then their shared shm buffer.
2. Client disconnects without sending surface destroy requests.
3. Server stops while the client and sixteen surfaces are still alive.

The actual client connects to the returned private socket, binds wl_compositor
version 1 and wl_shm, creates/attaches/commits surfaces and pending frame callbacks,
and synchronizes using real Wayland roundtrips. Barriers let the tests assert
both live resource counts and cleanup. No mock server or replayed content.

Final Godot test output: nine trials, 79 advancing frames, maximum observed pump
534 microseconds, zero failed assertions. This is a narrow test observation,
not an end-user latency or sustained performance benchmark. Socket directories
are removed after explicit and implicit destruction; repeated starts work.
`ldd` on the extension has libwayland-server and no EGL/GLES/Wayland-client or
legacy display library dependencies.

- `meson compile -C build`: passes.
- `meson test -C build --print-errorlogs`: 32/32 pass, including the new
  `compositor-runtime` regression.
- Standalone lifecycle built with `-fsanitize=address,undefined` and run with
  `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` and
  `UBSAN_OPTIONS=halt_on_error=1`: nine trials pass without sanitizer reports.
  Sanitizers cover the core/runtime/test, not Godot or system libraries.

Sanitizer reproduction from repository root:

```sh
c++ -std=c++20 -g -fsanitize=address,undefined -fno-omit-frame-pointer -I src \
  src/compositor-core.cpp src/compositor-runtime.cpp tests/test_compositor_runtime.cpp \
  $(pkg-config --cflags --libs wayland-server) \
  -o build-godot-probe/runtime-lifecycle-asan
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  build-godot-probe/runtime-lifecycle-asan build-godot-probe/runtime-client
```

## Exact continuation

P21-T39: adapt the existing xdg-shell and seat implementation to this runtime,
without dragging in display/camera globals. Prove a real fixture's initial
configure/ack/commit and seat discovery. Do not treat bare wl_compositor/shm
as enough to launch a normal terminal. Snapshot ownership, frame presentation,
buffer release, full pending/current buffer semantics and input delivery remain
open. The study still shows its inactive monitor preview; this server is tested
in the isolated Godot integration project, not yet started by the study scene.

Unrelated overnight edits remain uncommitted. Stage only recovery hunks in
`compositor-core.cpp`, its header and Meson; preserve the older serial/snapshot
and raw-ABI attempts separately. No host installation or service changes.
