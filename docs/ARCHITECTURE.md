# Elsewhere architecture transition

Project 22 will separate (1) Linux compositor and application services,
(2) world-independent desktop state and object/application anchoring, and
(3) Godot physical locations and visual presentation. See
[Decision 08](decisions/08-elsewhere-village-world.md). This is a target
boundary, not a claim that the current `world/scripts/study.gd` is already
separated. Preserve existing runtime behavior during extraction.

---

# Architecture: current prototype and target boundaries

## Current target: Decision 06

Project 21 replaces the custom visible renderer with a Godot world frontend,
while retaining the C++ libwayland-server core and headless protocol harness.
Godot owns the nested host window and scene; a GDExtension pumps the compositor
on its owning thread and provides owned CPU frame snapshots plus explicit seat
and configure commands. See [GODOT-INTEGRATION.md](GODOT-INTEGRATION.md).
The older snapshot choice in P4-T15 is superseded by P21-T11/T12. Subsequent
sections describe still-useful ownership boundaries, not an alternate work order.
Current recovery (2026-10-03): the Godot study renders imported furniture and
has a collision-based controller. The extracted C++ core now passes real fixture
socket/surface lifecycle, initial xdg-shell handshake and seat protocol checks
through Godot. Shared `seat.cpp` and `xdg-shell.cpp` are independent of legacy
input policy and rendering. Owned shm snapshots now pass byte-level Godot tests.
The study now pumps that runtime and presents changing real Weston terminal
output via ImageTexture (T41). T42 adds keyboard focus/delivery through that
runtime's seat and explicit world/application modes in Godot. T43 adds logical
pointer mapping, focused button/axis delivery and lifetime-tracked drag grabs.
T44 adds bounded xdg resize suggestions, committed geometry/size limits and
actual terminal reflow on host resize. Popup presentation remains open.
T48 adds a Godot application wheel/search overlay and a separate Python helper
for freedesktop metadata, installed icons and argument-vector execution. The
frontend still owns the compositor session and its launched child processes.
Only desktop-entry IDs persist in recents; PID/window associations are temporary.
This does not add a KDE service dependency or general application compatibility.
T65/T66 add a runtime-local logical output and a CPU surface-tree boundary in
`surface-tree.cpp`. Raw, synchronized cached and visible state stay compositor-
owned; Godot receives immutable composed toplevel snapshots with canvas origins.
Subsurface stacking and input regions route pointer events to the child while
world identities and keyboard focus remain associated with the root. The launch
helper supplies an isolated native-Wayland/software-rendering recipe for Chrome.
See [the graphical application handoff](handoffs/37-graphical-applications.md).
Godot maps physical US keys; the seat owns XKB and held-key state, filters events
to the focused client, and clears focus on destruction.
The original addon is isolated
because registration crashes. A separate standard-binding regression establishes
the binding boundary, reused by the study's `elsewhere_runtime` addon. See STATUS.md
for current evidence.

## Supplied prototype

Status: target design for incremental changes, not a claim of implementation.
Current legacy source uses C++20, Meson, libwayland-server, a nested Wayland
host window presented by PBuffer readback/shm, EGL/GLES2, and a headless test path.
The owner confirmed narrow navigation/orientation fixes on 2026-10-02; broader
terminal usability remains unverified (see handoffs/04-host-window.md). `src/display.cpp` currently combines host
setup, GPU resources, camera/panel presentation, and room rendering. Separate
these through bounded tasks; retain working behavior throughout.

## Responsibilities

| Subsystem | Owns | Must not own |
| --- | --- | --- |
| Compositor | Protocol objects, pending/current surface state, buffer lifetime, configure sequence, seat delivery | Persistent placements, reminder semantics, furniture |
| Runtime presentation | Live-window handles, surface-tree view, logical size, scale/transform, damage, GPU imports/textures | Persistent identity or automatic process-to-artifact guesses |
| World model | Stable entity IDs, room IDs, parent-relative transforms, slots, movable furniture, door state | Wayland resource pointers, GL IDs, process lifetime |
| Desktop shell | Resources/artifacts, launch recipes, associations, focus policy, search, navigation, reminders | Wire protocol implementations or direct GL calls |
| Renderer | Mesh/material instances, application presentation, picking, lighting, render scheduling | Destruction of client protocol objects or database writes |
| Host backend | Nested window/output, EGL context lifecycle, host input/focus/resize, headless output | Artifact identity and client-launch policy |
| Persistence | Versioned schema, transactions, migrations, durable references | Restoration of Wayland connections or arbitrary application memory |

These are boundaries within one executable, not a requirement for services,
threads, an ECS framework, or speculative plugin interfaces. Map existing code
to them, introduce the smallest adapter required by the next interaction, and
document the owner of every new handle.

## Identity and lifetime

- A **resource** references a normal file/URI or an explicit launch recipe.
- An **artifact** is a durable spatial representation of a resource, with a
  stable UUID, room/parent IDs, transform or slot, and presentation metadata.
- A **world entity** represents a desk, shelf, door, monitor, or other object.
  Stable IDs belong to data; mesh filenames and furniture labels are not IDs.
- A **live window** is an ephemeral binding to a connected application surface
  tree. Use an opaque runtime handle with invalidation/generation semantics.
  Internal compositor callbacks may use resource pointers with tracked lifetime;
  persistence and world code must not store those pointers.
- A **render snapshot/lease** exposes renderable content with explicit lifetime,
  dimensions, scale, transform, damage, and coordinate mapping. Choose its
  representation during P4-T15 rather than inventing an API in this document.

A client exit invalidates its runtime binding and focus, releases imports and
textures safely, and leaves the durable artifact in an inactive state. Reopening
can create a new binding; app ID, title, and PID are hints, not proof of instance
identity. Let the user assign ambiguous windows. A client process may own many
windows, and closing one window must not terminate all of them.

Screenshot tests may retain an explicit bounded snapshot. They must not keep
dead Wayland resources alive or grow an unbounded orphan list. An inactive
artifact is not evidence of a still-running application.

## Presentation and input

World mode uses camera movement, targeting, and object selection. Application
mode shows the chosen surface tree at a readable size inside the host window;
it does not inherently require scanout or client-requested fullscreen.
Keep application pixels unlit and protect text from scene post-processing.

For an in-world hit, map the ray intersection to panel UVs, then to logical
surface coordinates, accounting for content aspect ratio, window geometry,
buffer scale/transform, and any letterboxing. Use the same tested mapping for
full-size input. Deliver focus/enter/leave and release held keys/buttons during
mode/focus changes. The compositor sends protocol events; the shell chooses
the target. Related popups/subsurfaces follow their parent and remain usable.

## Graphics and assets

This paragraph describes the legacy renderer only; Decision 06/Godot is the
current visual implementation target. For legacy maintenance, continue with
EGL/OpenGL ES. GLES2 remains the baseline until a measured GLES3
experiment and decision justifies change. Blender exports glTF/GLB; implement a
documented subset first, with named limits for meshes, materials, textures, and
lights. Start with ambient/directional lighting. PBR, shadows, skeletal
animation, physics, and navigation libraries are later capability choices,
not immediate dependency requirements. Record asset provenance and licenses.

## Navigation and reminders

Search resolves an artifact's current room/world transform. A navigation graph
or navmesh supplies a route; emissive geometry presents it. Door state changes
route availability. Search and teleport remain available without walking.

Reminder data owns due time, timezone, recurrence, acknowledgement, and snooze.
The notification/list and optional creature are presentations of that data.
Creatures may navigate toward the player in world mode; they must not capture
application input, hide unsaved-work dialogs, or make deadlines inaccessible in
application mode. Persist deadline state independently of animation state.

## Decisions and non-goals for recovery

Decision 02 remains the implemented stack; Decision 05 reopens its suitability
assessment. A small wlroots experiment compares host backend, buffer import,
surface trees, and input integration while preserving the custom spatial shell.
Neither a successful experiment nor this document authorizes a wholesale rewrite.

Native session support, Xwayland client support, portals, multiple displays,
locking, and broad compatibility retain their later roadmap gates. No protocol
version may be advertised solely because generated headers support it; expose
only behavior that has been implemented and tested for that version.
