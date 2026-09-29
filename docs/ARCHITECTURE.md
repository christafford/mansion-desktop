# Architecture: current prototype and target boundaries

Status: target design for incremental changes, not a claim of implementation.
Current source uses C++20, Meson, libwayland-server, an X11 nested host window,
EGL/GLES2, and a headless test path. `src/display.cpp` currently combines host
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

Continue with EGL/OpenGL ES. GLES2 remains the baseline until a measured GLES3
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
