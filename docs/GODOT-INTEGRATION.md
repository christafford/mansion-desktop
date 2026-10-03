# Godot and the existing Wayland compositor

Target design under Decision 06; implement and verify through Project 21.
The Godot study now renders real furniture and supports movement (P21-T36).
The legacy `--room-camera` executable remains separate. The experimental
compositor addon is preserved but ignored because it crashes during registration.
The standard binding probe (P21-T37) is isolated under `tests/godot-binding/`.
The standard binding serves real wl_compositor/shm clients (T38), including
xdg-shell configure/ack/commit and seat v1/v4 keyboard setup (T39).
Owned frames (T40) and T12/T13 pixel presentation still require their own evidence;
the study does not start this server yet. See [STATUS.md](STATUS.md).

## Build and ownership

Pin a stable Godot release and compatible godot-cpp revision. Start with the
least complicated renderer that passes the visual target on the Steam Deck;
compare Compatibility and Mobile using actual scenes, driver support and
measurements. Don't depend on a RenderingDevice in a renderer that lacks it.
Put the engine project in `world/`, core library in an appropriate C++ source
boundary, adapter in `src/godot/`, and bootstrap/check commands in `tools/`.
The tested `MansionCompositorSession` owns the reusable server and protocol state;
`tools/check-godot-binding.sh` exercises it from a fresh isolated Godot project.
The shared `seat.cpp`/`xdg-shell.cpp` protocol library has no legacy display or
camera dependencies. Legacy input policy remains in `input.cpp`; its global
seat wrapper is separate from each runtime's seat. See
[handoff 18](handoffs/18-godot-window-protocol.md) for ownership and remaining limits.

Godot owns the visible host window and frame loop. GDExtension owns one compositor
instance with explicit start/pump/stop. Use a nonblocking pump on the owning
thread with a measured work budget to avoid starving rendering. Use one owner
for Wayland callbacks, client resources and shutdown; engine cleanup must not
invoke a callback after destruction. Keep the standalone headless executable
for existing tests. Avoid pulling the old host-window GL renderer into the
extension merely to reuse a texture: separate protocol/core and presentation.

## Minimum frame contract

Provide an opaque live-window ID with generation, logical size, buffer size,
format, stride, scale/transform, frame revision, damage and an owned pixel
snapshot. Expose no raw Wayland pointers to GDScript. Validate overflow, buffer
bounds and supported formats; normalize pixels into one documented engine
format. XRGB alpha is opaque; ARGB alpha handling and channel order are tested.
Respect wl_shm access brackets and buffer release; copied pixels remain valid
after commit replacement, client disconnect and buffer destruction.

Create ImageTexture when size/format changes; update it on valid new content.
Coalesce updates and eventually damage-aware copies without starving frame
callbacks. Record bytes copied/uploaded and CPU cost. A GLES texture handle or
Vulkan image cannot just be cast into a Godot RID. GPU sharing requires a later
separately verified synchronization/import design. Document shm-only compatibility
and test a real native software-rendered terminal first.

Render client content unlit inside the monitor aperture in world mode. In
application mode, show the same live content at a readable pixel scale with
defined letterboxing. Both views share one active-window binding and coordinate
mapping. Moving furniture moves the screen slot. Closing/remapping a client
invalidates focus and content immediately; an inactive artifact is visibly
inactive rather than displaying a stale screenshot as a live session.

## Input, configure and lifecycle

Build explicit physical-key mapping to evdev/XKB seat codes. Validate printable
keys, layout/modifiers, repeat, press/release and unsupported codes. Scope all
input to Godot's focused host window. World movement keys are consumed only in
world mode; application mode delivers ordinary text/shortcuts. Document one
reserved return key. Host-focus loss/mode changes synthesize appropriate releases
and clear ownership so no key/button sticks. Do not open global input devices.

Map Godot pointer coordinates through the displayed content rectangle, UVs,
orientation, buffer transform/scale and surface-local logical coordinates.
Test corners, center, aspect ratios, letterboxing and resized content. Send
configure sizes and obey acknowledge/commit ordering; do not stretch buffers
and claim the client resized. Audit existing stride/format/commit tests rather
than assuming historical task titles prove all cases.

Test create/map/unmap/remap/resize/destroy/relaunch, popup/subsurface trees and
per-window close behavior. Add deterministic bridge tests for ownership and
mapping, and a runtime extension load/unload test. Run relevant sanitizer
checks; `detect_leaks=0` is not leak verification. Real-client evidence uses
the compositor's private socket and explicit client backend, not a terminal
opened directly on the host or a prerecorded texture.

## Convergence

The visual scene can use a clearly labelled diagnostic screen while the bridge
is built. That diagnostic screen never satisfies live-terminal acceptance.
The integration gate requires the furnished room and real live terminal together:
typing, pointer/scroll, resizing, enter/leave application mode, host-focus loss,
close/relaunch and responsive world movement, with observed runtime evidence.
If rendering or GUI tools are missing, record a blocker and continue independent
eligible tests/assets/core work. Headless import alone cannot pass a visual gate.
