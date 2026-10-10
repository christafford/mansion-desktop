# Six exploration rooms beyond the gallery

Owner request 2026-10-08, starting from `e1deea6`, bounded to P21-T61.

## Implemented behavior

All six former closed doors lead to furnished rooms. Walking within 2.5m opens
a leaf inward over 450ms; it stays open for the session. There is no new key
binding and no automatic closing that could trap a person or carried panel.
Leaves retain collision, and walls, rails and wainscot have actual openings.
The widened 1.8m apertures accommodate the existing 1.4m application panels.
The study entrance remains doorless, and the oil lamps/burgundy gallery remain.

Facing away from the study, the destinations are:

| Door | Destination | Distinctive details |
| --- | --- | --- |
| First left | Long Library | Green walls, filled imported shelves, reading runner |
| First right | Atlas Room | Expedition map, flat-file drawers, brass armillary |
| Second left | Winter Garden | Glass roof, direct sun, checkerboard stone, plants/fountain |
| Second right | Cabinet Gallery | Wine walls, framed abstractions, brass/terracotta sculptures |
| Third left | Inventor's Room | Mechanical drawing, tools, open parts bins, metal workbench |
| End | Observatory | Wider chamber, star ceiling, meridian inlays, large armillary |

Side rooms are 3.8 × 6.4m, the observatory 9 × 8m, with ceilings at 3.3m.
Seven tables give each destination a place to put real applications. Existing
CC0 furniture/textures are instanced, with original authored joinery, turned
legs, curved instrument/fountain meshes, floor material and vector prints.
See [art provenance](../../world/art/README.md). No dependencies or downloads.

`elsewhere_rooms.gd` owns local room transforms and stable `elsewhere.*` room IDs.
The hallway's existing placement helper includes transformed room volumes and
small overlapping threshold volumes; full rotated bounds still have to fit.
Existing placement collision checks, live texture ownership, gravity, selection,
typing and application-mode transitions are reused. Applications opened while
in a room appear nearby. Existing instances retain their physical locations
while exploring and working elsewhere. These are **session-local** placements;
no save/restore, durable resource association or process restoration is added.
New room furnishings are fixed; the study's three movable furniture objects
retain their drag/push behavior and can be carried into the expansion.

## Verification and evidence

Agent-run, 2026-10-08, `e1deea6` plus T61. Godot 4.7.2 official ed1daf0bf,
Compatibility/OpenGL 4.6, Mesa 26.2.4, AMD Custom GPU 0405 (Steam Deck),
1280×800 GPU-rendered private Weston display. Existing 4× MSAA, lighting and
application color path retained. Physical input and human comfort: not observed.

The room test exercises all six approach-opening doors, actual controller walking
in/out while carrying a real terminal, floor/support landing, solid side walls,
independent entity identity, observatory table landing, real typing and return,
and a separately launched terminal in the library without moving the observatory
instance. Shell-written markers verify input reaches both independent shells.
The older hallway trial holds leaves closed as an explicit fixture to retain
closed-door collision coverage; the new room trial separately tests opening.
Its extreme rotated-bound assertion now uses the expanded exterior boundary.

Agent-inspected GPU views:
[library](../evidence/t61/library.png), [atlas room](../evidence/t61/atlas.png),
[garden](../evidence/t61/garden.png), [gallery](../evidence/t61/gallery.png),
[workshop](../evidence/t61/workshop.png), [observatory](../evidence/t61/observatory.png),
[named hall entrances](../evidence/t61/hall-entrances.png),
[open library entrance](../evidence/t61/open-library-door.png),
[observatory application](../evidence/t61/observatory-application.png),
[independent library application](../evidence/t61/library-application.png).
These are real runtime captures, not concept renders. Review led to distinct
warm/cool light, quieter light energy, library/gallery panel rails, enclosed map
cabinet, open parts bins and larger entrance lettering. Furniture has visible
floor contact, clear walking paths and distinct landmarks. Hard shadow edges
and specular highlights remain part of this renderer's current appearance;
this does not tick the broader final-art or human comfort gates.

Import/runtime/native helper validation passes; all 17 Godot checks and 7 Python
checks pass across the runs below. The full script did **not** finish in one
invocation: its launcher child was terminated at the existing 45-second limit,
then Godot reported a lost Weston connection. A bounded standalone run passed
all launcher assertions in 46 seconds. Existing test deadlines/assertions were
not relaxed; the new multi-room trial has its own 120-second allowance for six
walking/carrying routes. The remaining checks passed in separate GPU runs.

```sh
SDL_JOYSTICK_LINUX_CLASSIC=1 tools/validate-godot-project.sh
SDL_JOYSTICK_LINUX_CLASSIC=1 \
  GODOT_BINARY="$PWD/.tools/t61/godot-check.sh" tools/check-godot-study.sh
```

Full-run logs: `.tools/t61/suite.log`, `/tmp/elsewhere-study-check.dSgFhI`.
Final import/runtime: `.tools/t61/final-import.log`,
`/tmp/elsewhere-godot-check.aPN469`. Local test drivers `run.sh`, `remaining.sh`
and `godot-check.sh` are under `.tools/t61/`; the reproducible standalone recipe
is below (substitute `app_launcher`, `application_objects`,
`application_transition` or `application_gravity` for `elsewhere_rooms`):

```sh
SDL_JOYSTICK_LINUX_CLASSIC=1 \
ELSEWHERE_LAUNCHER_STATE_DIR="$PWD/.tools/t61/launcher-state" \
timeout -k 2s 120s weston --backend=headless --fake-seat --renderer=gl \
  --width=1280 --height=800 --socket=elsewhere-t61 --no-config --idle-time=0 \
  --log="$PWD/.tools/t61/weston.log" -- \
  /usr/bin/env WAYLAND_DISPLAY=elsewhere-t61 \
  "$PWD/tools/Godot_v4.7.2-stable_linux.x86_64" --display-driver wayland \
  --path "$PWD/world" --audio-driver Dummy --max-fps 60 \
  --script res://tests/elsewhere_rooms.gd
```

The full-run wrapper uses a 1600×1000 private display for the existing four-size
resize trial and 1280×800 for other graphical trials. The three pure mapping
checks use Godot headless. Product host-focus policy is unchanged.
Separate outcomes/logs are in `.tools/t61/remaining.log` and the per-test `.log`
files. Existing launcher, two-client object controls, animated entry/return and
four support-surface gravity checks retain their original assertions.

For fixed-view performance, add `-- --rooms-profile` to the standalone room
command. After each view settles it records 120 process-frame intervals, with a
live terminal and the same 60 FPS cap. `.tools/t61/profile.log` reports:

| View | Median ms | 95th percentile ms | Maximum ms |
| --- | ---: | ---: | ---: |
| Library | 16.667 | 16.844 | 24.509 |
| Atlas | 16.666 | 16.846 | 28.826 |
| Garden | 16.662 | 17.089 | 24.439 |
| Gallery | 16.677 | 16.872 | 17.915 |
| Workshop | 16.671 | 16.879 | 23.965 |
| Observatory | 16.670 | 16.856 | 24.592 |

Godot static allocations were approximately 256MiB; one contemporaneous Linux
RSS sample was 482MiB. This is bounded, stationary-view frame pacing in a private
GPU compositor, not input latency, a sustained traversal benchmark or the broader
performance gate. GPU memory is not included in the static allocation figure.
No C++ implementation changed, so no new Meson acceptance is inferred. No human
task was ticked and nothing was pushed. The unrelated roadmap T45 stays outside
this request.
