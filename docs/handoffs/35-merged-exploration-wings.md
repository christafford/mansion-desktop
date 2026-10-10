# Merged exploration wings

Owner request 2026-10-09, from `95ea216`, P21-T63. This is the first checkpoint
of the combined room-merging/grand-foyer request; P21-T64 continues afterward.

The first left entrance (former library) and second right entrance (former
gallery) are removed and sealed on both sides. The library and winter garden
form a 7.8 × 12.8m wing reached through the former garden door. Atlas and cabinet
gallery form another 7.8 × 12.8m wing reached through the former atlas door.
The inventor's room and end destination are retained at this checkpoint.
The hall retains all six oil lamps and has four remaining approach-opening doors.

Both copies of the dividing walls, their skirtings/cornices and attached trim
are removed. Matching floor, roof and front/back-wall strips bridge the 0.2m
construction gap, with no coplanar overlap between the two floor finishes.
Each pair receives a single merged placement volume, so full application/furniture
bounds can cross the old partition. Sealed entrances lose their threshold volume.
The six stable area IDs are preserved as zones inside the four physical rooms;
no live-window identity or persistence behavior changes.

Library bookcases now run along the outer wall. Art previously on removed walls
is rehung on surviving walls, and the map cabinet moves to the outer wall with
its drawers facing into the room. Contents remain movable as in T62. Distinct
wood/stone floors, wall colors and the garden's glass roof remain as cues within
the wider interiors. Existing study controls and live-client behavior are reused.

The new `merged_rooms.gd` graphical trial exercises both directions across each
former partition at three depths (12 crossings total), using actual controller
walking while carrying a real terminal. It checks sealed entrance collision,
remaining entrances, floor continuity, full-bound carrying, application identity,
landing and real shell typing afterward. The hallway test now expects four doors
and still checks all six shadow-casting oil lamps. The room trial exercises the
four remaining entrances; internal wing crossings have their separate trial.
An initial hallway check caught an accidentally omitted oil lamp during the door
list edit; the lamp was restored before the final checks.

Verified 2026-10-09 by the agent, `95ea216` plus T63, Godot 4.7.2 ed1daf0bf,
Compatibility/OpenGL 4.6, Mesa 26.2.4, AMD Custom GPU 0405, private GPU Weston
1280×800, existing 4× MSAA/60 FPS cap. Human input/comfort: **not observed**.
Agent-inspected [left wing](../evidence/t63/left-wing.png) and
[right wing](../evidence/t63/right-wing.png) show the opened interiors and
repositioned contents. These are live engine captures, not concept art.

Import/runtime/native-helper checks pass (`.tools/t63/final-import.log`,
`/tmp/elsewhere-godot-check.9Vcs6m`). The five affected graphical trials pass:
`hallway`, `elsewhere_rooms`, `merged_rooms`, `room_furniture`, and
`application_gravity`. Logs/drivers: `.tools/t63/check.log`, `check-final.log`,
`*-final.log`, `check.sh`, `run.sh`, `godot-check.sh`. A stale side-wall assertion
in the existing room trial initially targeted the removed partitions; it now
checks the surviving outer walls while the new trial separately requires open
crossings. No collision requirement was discarded. No C++ source changed.

Reproduce using the private Weston recipe in handoff 34, replacing its script
with `res://tests/merged_rooms.gd`, socket/output paths with `t63`, and requiring
`MERGED_ROOMS_OK crossings=12 sealed=2 failures=0` with no Godot errors. The
repository's study check now registers the merged-room trial with a 90-second
allowance; existing trial deadlines remain unchanged. Documentation links, task
IDs, shell syntax and diffs pass review. No dependencies or asset downloads.

Next: implement T64's foyer/stair/gallery as the second part of the same owner
request. This checkpoint is not the end of that assignment. Nothing was pushed;
placements remain session-local and the unrelated T45 gate remains open.
