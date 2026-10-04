# Godot and the existing Wayland compositor

Target design under Decision 06; implement and verify through Project 21.
The Godot study now renders real furniture and supports movement (P21-T36).
The legacy `--room-camera` executable remains separate. The experimental
compositor addon is preserved but ignored because it crashes during registration.
The standard binding probe (P21-T37) is isolated under `tests/godot-binding/`.
The standard binding serves real wl_compositor/shm clients (T38), including
xdg-shell configure/ack/commit and seat v1/v4 keyboard setup (T39).
Owned frame transport now passes exact client-byte tests through Godot (T40).
The study now starts that server and displays real Weston terminal output (T41).
T42 adds real keyboard input and a flat application view; T43 adds text selection
and pointer/scroll routing. T44 adds real client resize through xdg configure.
See [STATUS.md](STATUS.md).

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
[protocol handoff](handoffs/18-godot-window-protocol.md) and
[frame contract](handoffs/19-godot-owned-frames.md) for ownership and limits.

`tools/build-godot-runtime.sh` stages the same tested library in the world's
`addons/mansion_runtime/`; the run/import helpers invoke it automatically.
`terminal_screen.gd` owns a session and its one launched terminal process.
It chooses from `toplevel_handles()` and calls `snapshot(handle, after_revision)`
to avoid unchanged pixel copies. The monitor applies inverse buffer transforms,
fits the logical aspect ratio, and clears content on detach/destruction.
The unshaded screen shader compensates for the study's fixed Filmic tone curve;
this is tested for Compatibility at exposure/white 1, not a general renderer
color-management solution. See [live output handoff](handoffs/20-godot-live-terminal.md).

`application_mode.gd` implements explicit shell policy: Enter activates the live
terminal, Ctrl+Alt+Escape returns to world, and host focus loss clears activation.
Its TextureRect shares the monitor's current texture, displays native pixels
when they fit, and bypasses the 3D world environment. `keyboard_map.gd` maps
physical US Godot keys to Linux evdev codes. The runtime resolves only live,
mapped toplevel handles for `focus_keyboard`, `keyboard_focus_handle` and
`keyboard_key`. `seat.cpp` owns XKB, held keys, focused-client routing, late
keyboard enter and a surface-destroy listener. Duplicate presses are suppressed;
the Wayland client repeats using the advertised rate/delay. See
[keyboard handoff](handoffs/21-godot-terminal-keyboard.md).

`pointer_map.gd` derives the actual drawn rectangle from the flat TextureRect and
maps it to committed logical dimensions. The image is already upright; scale
and rotation must not be applied a second time. `application_mode.gd` routes
motion, Linux button codes and scroll through that session, with no world or
margin clicks. The seat owns focused-client delivery, bounded held-button state,
implicit grab and destruction cleanup. Mode/focus exit releases buttons before
leave. See [pointer handoff](handoffs/22-godot-terminal-pointer.md) for exact APIs,
validation and limitations: client cursors, clipboard and popup menus remain
unsupported even though real text highlighting and scrollback work.

`request_resize(handle, width, height)` suggests a bounded window-geometry size
through xdg configure; the client chooses when/what to commit. `window_state`
exposes committed geometry/limits separately from sent/acked/committed serials.
Both frontends apply xdg metadata after surface state. The Godot viewport follows
the host size; application mode debounces resize, reserves UI/shadow space and
allows for client size increments. Old pixels and logical pointer mapping remain
valid until the client commits. No texture scaling is treated as a client resize.
See [resize handoff](handoffs/23-godot-terminal-resize.md) for API bounds,
configure backpressure, native-pixel evidence and unsupported cases.

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
