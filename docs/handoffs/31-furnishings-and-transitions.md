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

## Continuation

T57 wall paneling, T58 independent launcher instances and T59 live transitions
remain. No human usability or broad roadmap gate is claimed.
