# Owner-requested room details and physical objects

Started 2026-10-05, resumed 2026-10-06, starting revision `35d6b3d`. This bounded
request covers T53–T55; it does not authorize a broader roadmap run or complete
a human gate.

## T53: ceiling fittings and wall decoration

Two original 1.38m enamel twin-tube fittings provide shadowed downlight, with
asynchronous small ballast variation (measured multiplier 0.94655–0.99976 over
60 simulated seconds). Tube emission follows the same variation. The original
unshadowed room fill was removed, daylight reduced and ambient retained. No
renderer, dependency or application-color changes. The wall details are two
original botanical SVG prints, a clock showing opening time, and a noticeboard.
See [original asset provenance](../../world/art/README.md).

Verified this session: `SDL_JOYSTICK_LINUX_CLASSIC=1 tools/validate-godot-project.sh`,
graphical `res://tests/study_details.gd` and `res://tests/screen_color.gd`.
All passed; 226198 sampled pixels changed in the ceiling-shadow comparison,
and all 11 synthetic client-color patches retained their expected RGB values.
The new scene test is part of `tools/check-godot-study.sh`.

Agent-observed GPU captures at 1280×800, Godot 4.7.2 official ed1daf0bf,
Compatibility OpenGL 4.6, Mesa 26.2.4, AMD Custom GPU 0405:
[arrival](../evidence/t53/arrival.png), [desk](../evidence/t53/desk.png),
[reverse](../evidence/t53/reverse.png), [ceiling](../evidence/t53/ceiling.png),
[shadows on](../evidence/t53/shadows.png) and
[ceiling shadows disabled](../evidence/t53/shadows-disabled.png).
Cameras are fixed in the scene test; flicker is frozen only for comparison.
Initial captures exposed excessive floor light and cropped BoxMesh artwork UVs;
energy was reduced and artwork moved to full-UV quads, then captured again.
The images show actual geometry, downward chair/desk shadows, framed artwork,
noticeboard and live terminal. These are agent observations, not owner comfort
or performance acceptance; remaining coarse shadow/texture sampling and broad
finished-room gates are not claimed resolved.

Implementation uses Godot's documented
[SpotLight3D](https://docs.godotengine.org/en/stable/classes/class_spotlight3d.html)
direction and shadow support; no new asset or runtime dependency.

## T54: pushable chair

The imported chair now sits under a 7kg upright RigidBody3D. Six fitted box
colliders describe its seat/back/legs; the detailed triangle mesh is visual
only. Walking contact applies a bounded horizontal impulse based on relative
speed. Floor friction and damping bring it to rest. Pitch/roll locks keep it
usable as furniture, while room and desk collision remain solid.

Verified this session: import/runtime validation and expanded graphical
`res://tests/study_smoke.gd -- --no-terminal`, `STUDY_SMOKE textured_surfaces=56
failures=0`. It checks actual walking displacement, player/chair separation,
coasting, upright floor contact, desk blocking and a separate walk against the
right wall; all previous movement/capture/focus assertions also pass.

## T55: released application gravity

Panels now sweep a collision box matching their visible frame downwards under
gravity, retain their heading and stop at the first support. They stay upright
for readability. Continued contact probes allow a panel to fall again after a
support moves away. Held panels suspend falling; cancel restores transform and
falling state. Simply selecting an untouched panel leaves its initial placement
alone. Placement queries exclude the panel itself, and physics/picking separate
room/furniture, player and panels. Window lifetime and unlit pixels still belong
to the existing compositor/presentation path.

The real-client gravity trial checks floor, desk, bookcase top and chair seat
heights against the actual scene, including a panel edge landing where its
center misses the chair. It pushes the supporting chair away, re-grabs/lifts,
cancels, verifies continuing client updates, types and closes the client.
`APPLICATION_GRAVITY_OK supports=4 failures=0`; the real two-client trial also
passes `APPLICATION_OBJECTS_OK clients=2 failures=0` with updated falling/drop
expectations and unchanged walking/rotation/cancellation/input assertions.

Initial gravity test placements correctly caught the desk lamp; the desk-only
fixture was moved to clear it. A single chair impulse sometimes left a sliver of
seat under the panel. The final test uses the controller's sustained bounded
push and explicitly requires 0.8m of chair clearance before checking the fall.
Neither case required weakening the collision behavior. Two initial evidence
cameras were outside room walls; camera positions are now bounded inside the room.

The first broad suite lost host focus during the keyboard trial. An isolated
retry passed all assertions. The keyboard harness now explicitly acquires and
checks actual host focus before injection, matching the existing pointer and
launcher tests. Focus-loss policy in the product remains unchanged.

Full suite passed 2026-10-06 from `357fe7d` plus this T55 change:

```sh
SDL_JOYSTICK_LINUX_CLASSIC=1 \
  GODOT_BINARY="$PWD/.tools/t51/godot-check.sh" tools/check-godot-study.sh
```

The local wrapper described in [handoff 28](28-godot-natural-navigation.md) uses
the host input seat except for `terminal_resize.gd`, which runs in a private
1600×1000 GPU-rendered Weston desktop. This preserves the host's display limits.
Logs: `.tools/t55/suite-complete.log` and `/tmp/mansion-study-check.4TMHgU`.
Native helper build, import/runtime validation, seven Python tests and all 13
Godot study trials pass. The keyboard trial checked 11163 client pixels, pointer
selection/scroll passed, four actual terminal sizes passed, two live clients
retained their interaction, and `APPLICATION_GRAVITY_OK supports=4 failures=0`.
C++ implementation was unchanged. SVG XML, new task IDs, documentation links,
shell syntax and diffs also pass validation.

Agent-observed 1280×800 rendered contacts, same GPU/renderer as above:
[floor](../evidence/t55/floor.png), [desk](../evidence/t55/desk.png),
[bookcase](../evidence/t55/bookcase.png), [chair edge](../evidence/t55/chair.png).
The panel bottoms meet the visible surfaces; real changing terminal content
remains present. These images and injected checks do not establish human
readability, flicker comfort or a performance gate. Placements remain session
local. No new dependencies, host service changes or remote push.

## Continuation

T53–T55 are complete. The general roadmap next task remains T45 targeted client
close; this bounded request does not start it or a broader roadmap run.
