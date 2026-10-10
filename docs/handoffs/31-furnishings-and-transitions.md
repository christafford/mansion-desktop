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

## T59: live application flight

Entry and return interpolate a live unlit 3D surface over 220ms with smooth
acceleration/deceleration. The world panel retains its physical position and
identity; the camera/player never moves. The proxy shares the panel's shader
and changing client texture. The native endpoint uses the actual application
rectangle projected at camera depth, with ordinary native-size presentation
restored at completion. The original visuals are hidden only during the flight.
Keyboard focus starts immediately; pointer clicks wait until the image settles.
Return restores world input immediately and follows the current panel placement.

Rapid reversals begin at the current interpolated pose. Focus loss cancels the
flight and restores world visuals. New launches can wait a frame for their room
panel to appear. Client loss clears presentation; launcher retargeting cancels
the old animation before starting the new one. Resize recomputes the endpoint.
No compositor or persistent-identity changes.

Focused graphical test passed 2026-10-06: `APPLICATION_TRANSITION_OK failures=0`.
It checks intermediate motion, shared live material, both exact endpoints,
unchanged player/physical-panel transforms, native pixel equality, changing
client revisions during return, reversal/re-entry, resizing, relocated return
target, focus loss and actual shell exit. Import/runtime also passed after
fixing a GDScript inferred-boolean type error. Logs: `.tools/t59/import-fixed.log`
and `transition-fixed.log`. The first broad suite passed through keyboard input,
then exposed two pointer fixtures that clicked during the new entry animation.
Those fixtures now wait for and assert the visible input endpoint before testing
held-drag cleanup. All original selection, scroll, focus-loss and disconnect
assertions remain. The new flight test explicitly checks that clicks during
motion cannot grab the client. Both focused tests pass again (`pointer-fixed.log`,
`transition-pointer.log`).

Agent-inspected GPU captures at 1280×800, Godot 4.7.2 Compatibility/OpenGL,
Mesa 26.2.4, AMD Custom GPU 0405: [entry intermediate](../evidence/t59/enter-middle.png),
[return intermediate](../evidence/t59/return-middle.png),
[native application endpoint](../evidence/t59/application.png).
The application enlarges toward the view and recedes into the visible room;
the return capture contains live changing shell output. These are rendered
agent observations, not human animation-comfort acceptance.

Projection/interpolation follow the engine's documented
[Camera3D projection](https://docs.godotengine.org/en/stable/classes/class_camera3d.html#class-camera3d-method-project-position)
and [Transform3D interpolation](https://docs.godotengine.org/en/stable/classes/class_transform3d.html#class-transform3d-method-interpolate-with).

## Full verification and continuation

Verified 2026-10-06 from `c60d037` plus T59:

```sh
SDL_JOYSTICK_LINUX_CLASSIC=1 \
  GODOT_BINARY="$PWD/.tools/t51/godot-check.sh" tools/check-godot-study.sh
```

Native helper build, import/runtime validation, all 15 Godot trials and 7 Python
tests pass. Full log: `.tools/t59/suite-final.log`; per-trial logs:
`/tmp/elsewhere-study-check.VO36ka`. The existing wrapper uses the host input seat
and a private 1600×1000 GPU Weston desktop for the four actual client resize
cases; see [handoff 28](28-godot-natural-navigation.md). Keyboard tested 11163
client pixels; resize tested 8573. Furniture, Terminal/Vim, independent-instance,
transition and all four gravity support checks pass together. No C++ source
changes. Documentation links/task IDs, manifest JSON, shell syntax and diffs
also validated. No new dependencies, host changes or remote push.

T56–T59 are complete. The broader roadmap next task remains T45 targeted client
close, outside this bounded request. Human comfort/usability gates remain
unobserved; [the acceptance procedure](../ACCEPTANCE.md) is available for a person.
