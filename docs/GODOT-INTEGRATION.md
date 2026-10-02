# Godot and the existing Wayland compositor

Target design under Decision 06; implement and verify through Project 21.
A separate placeholder Godot project and draft adapter exist, but no integrated
frontend is working. The legacy `--room-camera` executable does not launch Godot.
See [STATUS.md](STATUS.md), reopened T00/T01/T10 and unfinished T11 in
[TASKS.md](TASKS.md).
Preserve existing tests and the headless path while completing the adapter.

## Build and ownership

Pin a stable Godot release and compatible godot-cpp revision. Start with the
least complicated renderer that passes the visual target on the Steam Deck;
compare Compatibility and Mobile using actual scenes, driver support and
measurements. Don't depend on a RenderingDevice in a renderer that lacks it.
Put the engine project in `world/`, core library in an appropriate C++ source
boundary, adapter in `src/godot/`, and bootstrap/check commands in `tools/`.
The `world/` and `src/godot/` paths now exist, but the core-library target and
working bridge do not. Repair existing run/validation tools rather than treating
their successful exit or comments as acceptance. Register an actual Godot class
through a `.gdextension` resource and prove it can serve a fixture client; do
not wait for a Project Settings UI to enable the draft `extension.toml`.

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
