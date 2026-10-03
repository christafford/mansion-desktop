# Godot window protocols — 2026-10-03

Historical T39 checkpoint (`821e7fe`). Continue from
[handoff 19](19-godot-owned-frames.md) after the completed T40 frame transport.

P21-T39 adapts the existing seat and xdg-shell implementation to the recovered
runtime. Starting revision: `5f7de10` plus preserved overnight edits. This is
protocol evidence from actual fixture clients, not real terminal usability.
The owner approved the recovered room's appearance in this session (“finally!
that looks great”). No human input/terminal acceptance task is checked by that.

## Implementation and ownership

`seat.cpp` owns protocol resources, XKB keymap/state and client keyboard/pointer
lists. `input.cpp` keeps legacy host/camera/script policy and its global seat
wrapper. `CompositorRuntime` owns an independent seat and shell, links the seat
to its existing core, and pumps them on Godot's main thread. Stop destroys clients
before shell, seat, compositor and display. No new dependency was introduced:
Wayland protocols/scanner and xkbcommon are already project dependencies.

The same shared protocol code serves both legacy and recovered frontends.
Seat resources honor their negotiated versions; keymap fd includes the NUL,
its generated string is freed, repeat events obey version rules, release
requests have implementations, and resource listeners unlink/free immediately.
Missing keymap creation fails startup rather than leaving an unusable seat.

The existing xdg-shell now sends one initial configure per constructed role,
tracks outstanding serials (including resize/restore), rejects reused/unknown
acks and commits of buffers before acknowledgement, and validates positive
geometry. Toplevel removal unregisters any list position. Unconstructed xdg
surface destruction is safe. Role objects free with their owning resources;
shell teardown destroys remaining resources rather than deleting live userdata.
No legacy renderer is linked into the recovered extension.

The old EGL readback fixture skipped the initial empty commit/configure/ack.
The stricter server rejected it. Its sequence now matches the shm fixture;
the existing rendering and screenshot assertions were preserved.

## Checks and limits

Commands run against this task's final code:

```sh
meson compile -C build
meson test -C build --print-errorlogs
tools/check-godot-binding.sh
```

The 32 Meson tests pass. The binding check passes fresh import and 300 standard
native calls, then 16 standalone and 16 Godot client trials. Each binds seat v1
and v4, parses both received XKB maps, checks versioned name/repeat events and
pointer/keyboard release, creates 16 toplevels, receives initial configure,
acks and commits real shm buffers twice without a configure loop.

Eight modes repeat twice: orderly destruction, abrupt disconnect, server stop,
stale ack, unknown ack, unconfigured buffer, commit without role and zero-width
geometry. Invalid requests must produce the exact xdg_surface error. Orderly
destruction removes alternate windows first; barriers assert counts 16, 8, 0.
Role-less destruction, pending frame callbacks, shared buffers and implicit
native/Godot object destruction are covered. These callbacks are cleaned up,
not yet completed by a presentation pipeline.

Final Godot output: 194 advancing frames, maximum observed pump 1,616 µs,
zero failed assertions (`build-godot-probe/lifecycle.log`). These are narrow
test measurements, not a product latency claim. `ldd tests/godot-binding/bin/libmansion_probe.so` lists
wayland-server and xkbcommon, with no EGL/GLES/wayland-client dependency.

Standalone ASan + UBSan + enabled leak detection passes all 16 trials:

```sh
cc -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  $(pkg-config --cflags wayland-server) -c build/xdg-shell-protocol.c \
  -o build-godot-probe/xdg-protocol-asan.o
c++ -std=c++20 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  -I src -I build src/compositor-core.cpp src/compositor-runtime.cpp \
  src/seat.cpp src/xdg-shell.cpp tests/test_compositor_runtime.cpp \
  build-godot-probe/xdg-protocol-asan.o \
  $(pkg-config --cflags --libs wayland-server xkbcommon) \
  -o build-godot-probe/runtime-lifecycle-asan
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  build-godot-probe/runtime-lifecycle-asan build/runtime-client
```

Godot and system libraries are not sanitizer-instrumented. Protocol error logs
are expected for the explicit negative trials; sanitizer reports are not.
The existing editor import workaround remains documented in TOOLCHAIN.md.
All 62 Node plugin tests pass; its parser recognizes T36–T39 as complete and
T40 as open. Changed Markdown links and unique task IDs were validated.

## Exact continuation

P21-T40: correct pending/current shm state and deliver owned frames through the
standard binding. Follow the concrete task in TASKS.md. The current core still
has incomplete buffer replacement/null-attach semantics and no snapshot copying,
release/presentation pipeline or exposed Godot input commands. Do not launch a
terminal and mistake connection for a functioning desktop. Subsequent work must
put real pixels on the monitor, then prove ordinary input/focus/resize/close.

Cursor surfaces, popups, subsurfaces, unmap/remap and comprehensive protocol
conformance remain separate open work. Seat extraction preserves the existing
US layout; this task does not implement configurable layouts or input delivery.
The study still shows its inactive preview; no new visual quality claim.

Unrelated overnight changes remain uncommitted. The pre-task copies of the dirty
Meson/core files are in `.tools/recovery/t39/`; stage only recovery changes.
No host services, packages or desktop session were modified.
