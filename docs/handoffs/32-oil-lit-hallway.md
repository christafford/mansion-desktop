# Doorless study entrance and oil-lit hallway

Owner request 2026-10-08, starting from `139214d`, bounded to P21-T60.

## Implementation

The wall opposite the desk (+Z) now has a 1.7m-wide, 2.5m-high opening. Solid
wall, veneer and skirting are all split around it; wood jambs and a low brass
threshold finish the opening. The connected gallery is 14m long and 2.8m wide,
with a 3m ceiling. When leaving the study, left is +X: three doors at z=7,11,15;
right is -X: two at z=9,13; the sixth closes the far end at z=18.5. All leaves
are detailed, static and collidable, with wall backing. No opening interaction.

Six brass oil lamps use original revolved reservoir, glass chimney, burner and
flame profiles, oval plates and brackets. Warm shadowed lights and small
asynchronous flame variation illuminate walnut joinery, plaster, paneled doors
and burgundy carpet with woven variation and narrow borders. Existing walnut
and plaster photos are reused. Original source/provenance is recorded in
[world art](../../world/art/README.md); no new downloads or dependencies.

Placement uses three overlapping inset volumes for study, threshold and hall.
Full transformed object bounds fit in a volume; existing overlap/collision
queries still apply. Applications and movable furnishings therefore use the
same carry controls across the opening. Application lighting/input/presentation
remain separate from the world lighting and geometry.

## Verification

Initial import/runtime passed. The first graphical hallway trial passed six-door
layout, six shadowed lamps, actual walking through the opening both ways, all
closed side doors/end door and wall collision, live terminal carrying across
the threshold, landing and activation/return. Initial fixed renders exposed
excessive lamp illumination and opaque-looking chimneys. Light energy was
reduced, glass made clearer, and the backplates refined into oval fittings.

The old application bounds regression tested the now-open front-wall center.
Its solid-wall rotation fixture was moved beside the opening; extreme clamping
still checks the study's solid back/right/ceiling boundaries. The new hallway
trial adds far-hall full rotated-bound checks. No collision assertion was removed.

The final hallway trial also passes (`HALLWAY_OK doors=6 lamps=6 failures=0`).
Agent-observed fixed renders at 1280×800, Godot 4.7.2 official ed1daf0bf,
Compatibility/OpenGL 4.6, Mesa 26.2.4, AMD Custom GPU 0405:
[entrance](../evidence/t60/entrance.png), [gallery](../evidence/t60/gallery.png),
[back toward study](../evidence/t60/return.png),
[oil lamp](../evidence/t60/oil-lamp.png), [door](../evidence/t60/door.png).
The refined views show an unobstructed doorway, continuous carpet, the requested
door rhythm, visible glass/flames and modeled brass/wood details. Hard point-light
shadows and the existing renderer's texture/shadow sampling remain visible;
no broader final-art or human comfort/performance gate is claimed.

All 16 Godot study trials and 7 Python tests passed across this session's host
and isolated runs. The initial full command was:

```sh
SDL_JOYSTICK_LINUX_CLASSIC=1 \
  GODOT_BINARY="$PWD/.tools/t51/godot-check.sh" tools/check-godot-study.sh
```

It passed native-helper build, import/runtime, seven Python tests and the first
12 Godot trials, including hallway, client colors, keyboard/pointer and four
actual resize cases. Logs: `.tools/t60/suite.log`,
`/tmp/elsewhere-study-check.Q2aZJj`; import `/tmp/elsewhere-godot-check.gKsJ0x`.
The launcher then lost actual host focus (`host_focus=false window_focus=false`)
and its shell assertions failed; a host-seat retry also lost focus. This is
recorded as a failed run, not silently counted as a pass. Product focus policy
was not changed to bypass it.

Launcher and the three remaining application trials were then run successfully
in private GPU Weston desktops, using the same installed engine and scripts:

```sh
SDL_JOYSTICK_LINUX_CLASSIC=1 timeout -k 2s 65s weston \
  --backend=headless --fake-seat --renderer=gl --width=1280 --height=800 \
  --socket=elsewhere-t60-launcher --no-config --idle-time=0 \
  --log="$PWD/.tools/t60/weston-launcher.log" -- \
  /usr/bin/env WAYLAND_DISPLAY=elsewhere-t60-launcher \
  "$PWD/tools/Godot_v4.7.2-stable_linux.x86_64" --display-driver wayland \
  --path "$PWD/world" --audio-driver Dummy --max-fps 60 \
  --script res://tests/app_launcher.gd
```

The same recipe used socket `elsewhere-t60-apps` for `application_objects.gd`,
`application_transition.gd` and `application_gravity.gd`. Isolated launcher
history stayed under `.tools/t60/`. Logs: `launcher-isolated.log`,
`application_objects-isolated.log`, `application_transition-isolated.log`,
`application_gravity-isolated.log`; local sequential driver `check-isolated.sh`.
All have zero failures. These are GPU-rendered, injected-input trials, not
human observation or Godot's headless renderer. Existing two-client interaction,
entry/return, support removal and all four landing surfaces remain verified.

Documentation links, unique task ID, manifest JSON, shell syntax and diffs pass.
No C++ implementation changes, new dependencies, host services or remote push.
T60 is complete. The general roadmap's T45 remains outside this bounded request.
