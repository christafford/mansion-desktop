# Owned client frames — 2026-10-03

P21-T40 builds on `821e7fe` and the preserved overnight tree. Real Wayland shm
pixels now cross the recovered native/Godot boundary as owned data. This is
frame transport acceptance, not an image on the study monitor or a usable terminal.

## Ownership and frame contract

`compositor-core.cpp` copies a committed supported shm buffer under
`wl_shm_buffer_begin_access`/`end_access`, then sends release and forgets the
client buffer pointer. Only a pending attachment retains a destroy listener.
Replacing an uncommitted attachment does not read/release the superseded buffer.
An explicit null attach produces an empty frame; an ordinary commit preserves
content. Destroying a pending buffer is unspecified by Wayland; this core
preserves the previous owned content. Destroying a released buffer does not
change the copy. Detach resets the xdg initial handshake for remapping.

`CoreFrameState` is owned only by the extracted core surface. Its pointer is
added to the shared surface structure without adding nontrivial vector ownership
to legacy surfaces. The legacy renderer continues to use its existing path.
The runtime consumes committed frame callbacks after dispatch; uncommitted
callbacks stay pending and committed callbacks preserve request order. This is
CPU bridge pacing, not a display presentation/scanout timestamp.

`surface_handles()` returns runtime-only positive handles for all live surfaces,
including surfaces with no pixels or no toplevel role. Handles are allocated
monotonically across sessions/objects in the loaded native module and are never
persisted. `snapshot(handle)` is empty for an unknown handle or before content
exists. C++ returns `shared_ptr<const FrameSnapshot>`; Godot gets a dictionary
with a copied PackedByteArray. Previously returned frames survive replacement,
disconnect and restart; live enumeration/lookup determines current validity.
Do not display a retained frame as a still-running application after removal.

Snapshot fields:

| Field | Meaning |
| --- | --- |
| handle, revision | Runtime identity and content/committed metadata revision |
| mapped | Has owned surface content; does not assert an xdg window role |
| width, height, stride | Buffer dimensions and packed output stride (`width * 4`) |
| format, pixels | RGBA8, top-left buffer row order, straight alpha |
| source_format, source_stride | Wayland ARGB8888/XRGB8888 format and original padded stride |
| scale, transform | Committed buffer interpretation; defaults 1 and normal |
| logical_width, logical_height | Transformed dimensions divided by scale |

XRGB becomes opaque. ARGB is converted from premultiplied to straight alpha;
zero-alpha RGB becomes zero. Pixel rows remain in **buffer coordinates**. The
presentation layer must apply the inverse Wayland transform and logical scale;
T40 tests all eight metadata values but does not claim rotated GPU output.
Full-frame copying subsumes damage; no partial-copy optimization is claimed.
Each frame is capped at 64 MiB and current compositor-owned frames at 256 MiB.
Caller-retained old frames and Godot copies are separate allocations. Unsupported
formats/buffer types and excess allocation requests fail explicitly. Opaque/input
regions, popups/subsurfaces and general compositor conformance remain open.

Protocol references used during implementation:
[Wayland surface/buffer specification](https://wayland.freedesktop.org/docs/html/apa.html)
and [server shm access API](https://wayland.freedesktop.org/docs/html/apc.html),
also checked against the installed `/usr/share/wayland/wayland.xml`.

## Verification

Commands:

```sh
meson compile -C build
meson test -C build --print-errorlogs
tools/check-godot-binding.sh
```

All 33 Meson tests pass. The binding script still checks 300 native probe calls
and 16 standalone + 16 Godot protocol lifecycle trials. It additionally runs
`frame-client` against the standalone runtime and actual Godot session: two
sessions each, 32 synchronized pixel/state barriers each, plus six negative cases
each. This is real socket traffic and client-written shm, not direct setters
or synthetic snapshot injection. The final Godot frame trial advanced 156
engine frames with zero failed assertions. No rendered latency claim is made.

Assertions cover asymmetric ARGB/XRGB pixels, padding removal, transparent and
partial-alpha pixels, buffer-size changes, mutation/destruction after release,
superseded pending attachments, uncommitted scale/transform/callback state,
pending-buffer destruction, detach/remap, all transform metadata, callback order,
immutable retained frames, Godot byte-array mutation isolation and stale handles
across restarts. Client assertions verify buffer-release and frame-done events.

Negative cases check exact protocol error codes for invalid scale, invalid
transform, an oversized frame, unsupported RGB565, non-divisible scale and a
truncated backing file. The last exercises Wayland's guarded shm access: the
client fails while the compositor and Godot keep running.

Standalone frame and protocol lifecycle checks also pass with AddressSanitizer,
UBSan and enabled leak detection. Sanitizers cover project core/runtime code,
not Godot/system libraries. Reproduce after the normal build:

```sh
cc -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  $(pkg-config --cflags wayland-server) -c build/xdg-shell-protocol.c \
  -o build-godot-probe/xdg-protocol-asan.o
c++ -std=c++20 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  -I src -I build src/compositor-core.cpp src/compositor-runtime.cpp \
  src/seat.cpp src/xdg-shell.cpp tests/test_frame_snapshot.cpp \
  build-godot-probe/xdg-protocol-asan.o \
  $(pkg-config --cflags --libs wayland-server xkbcommon) \
  -o build-godot-probe/frame-snapshot-asan
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  build-godot-probe/frame-snapshot-asan build/frame-client
```

For the lifecycle sanitizer check, substitute `tests/test_compositor_runtime.cpp`,
`runtime-lifecycle-asan` and `build/runtime-client`. Expected negative-case
protocol errors are not sanitizer failures. Logs are under
`.tools/recovery/t40/` and `build-godot-probe/{lifecycle,frames}.log`.
Changed Markdown links, unique numeric task IDs and the actual autocontinue
parser were validated; it identifies T41 as the next open recovery task.

## Exact continuation

P21-T41: install the recovered standard binding into the study's development
launch path; select real toplevel content, update an unlit ImageTexture on the
monitor, launch an installed native software-rendered terminal on the private
socket, and capture actual changing output. Keep the old crashing addon ignored.
The existing study/controller/art remain intact. See TASKS.md for acceptance.
Input delivery/focus, application mode, resizing/close and terminal usability
will still need their own bounded checks after visible live content works.

Dirty overnight edits remain separate. Pre-task core/header/Meson copies are in
`.tools/recovery/t40/`. No system installation or host desktop changes.
