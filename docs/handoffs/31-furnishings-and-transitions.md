# Furnishings, wood walls, application instances and transitions

Owner request started 2026-10-06 from `e618487`, bounded to T56–T59.

## T56: shared movable furnishings

Chair, plant and bookcase now share the existing panel drag controls: left-drag,
wheel depth, both-buttons wheel yaw, WASD carrying and Escape cancellation.
Walking pushes all three. Fitted upright rigid bodies use floor friction and
bounded walking impulses; the bookcase is heavier. Picking respects nearer
room surfaces and application panels. Plant foliage is pickable; its pot is the
solid collision body. Desk and its accessories stay fixed. Placement is session
local, as with existing panels.

Verified 2026-10-06 using Godot 4.7.2 Compatibility/OpenGL on AMD Custom GPU 0405:
import/runtime validation, new graphical `furniture_drag.gd -- --no-terminal`,
existing `study_smoke.gd -- --no-terminal` and real-client `application_gravity.gd`.
All passed. The new test covers all three items, carrying, depth, yaw, cancel,
release/focus/launcher cleanup, desk obstruction, floor settling and actual
controller pushes. The existing gravity trial still passes all four support
surfaces, including the movable bookcase and removal of the chair support.
Logs: `.tools/t56/import-final.log`, `verified.log`, `controller.log`, `gravity.log`.
Initial test teleports raced physics synchronization; the fixture now waits
while frozen before and after repositioning. The furniture-only trial passes
`--no-terminal` so a real panel does not legitimately block its fixture space.

## T57: walnut wall paneling

Reused the already verified Poly Haven walnut veneer photo with meter-scaled
vertical grain on separate quads, 65cm panel bays, recessed joints and rails.
The solid wall backing retains collision and the real window opening. Layered
veneer does not cast duplicate self-shadows; solid walls still occlude lights.
No new assets or dependencies. This supersedes the earlier plaster direction.

Verified 2026-10-06: import/runtime, fixed-view scene trial (225641 sampled
shadow-difference pixels), all 11 client-color patches and controller/window/
wall collision regressions pass. Logs: `.tools/t57/`.
Agent-observed GPU renders at 1280×800, same renderer as T56:
[arrival](../evidence/t57/arrival.png), [reverse](../evidence/t57/reverse.png).
The visible room has continuous vertical wood grain, distinct panel seams and
horizontal joinery, with the opening, decorations and downward lighting intact.
These are agent visual observations, not human comfort/performance acceptance.

## T58: independent application instances

Launcher selections always invoke a fresh desktop-entry launch. Window labels
now map each transient window to its desktop ID, allowing multiple siblings;
closed bindings are pruned and recents remain deduplicated by application.
Existing room panels still activate their own window. Applications that enforce
their own singleton policy remain subject to that client policy.

Verified 2026-10-06: import/runtime and expanded graphical launcher trial pass,
`APP_LAUNCHER_OK entries=16 failures=0`. Actual UI selection creates a second
Weston terminal alongside the original; both keep their labels and accept shell
input. A variable set in one is absent in the other. Closing the second leaves
the original alive, removes only its binding, and another launch accepts input.
Vim, keyboard ownership, search, wheel layout, MRU persistence and owned-process
cleanup also pass. Logs: `.tools/t58/import.log`, `launcher.log`.

## Continuation

T59 live entry/return transitions remains. No human usability or broad roadmap
gate is claimed.
