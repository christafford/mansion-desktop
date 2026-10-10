# Longer rooms and movable antique furnishings

Owner request 2026-10-08, starting from `58d2be5`, bounded to P21-T62.

## Implemented behavior

Rooms 01–04 (library, atlas room, winter garden and cabinet gallery) are now
3.8 × 12.8m, twice their former depth. Room 05 remains 3.8 × 6.4m; the
observatory remains 9 × 8m. Floors, walls, placement volumes, roof framing,
paneling and lighting extend through the longer rooms. Furniture and wall art
are distributed along the longer routes, with worktables toward the far ends.
The atlas instrument and gallery stands leave a clear central application route.

The library has two runs of four independently movable antique bookcase bays:
black framing and crown mouldings, warm vertically planked interiors/shelves,
metal uprights and collars, books and small brass vessels. A burgundy runner
leads to a black reading table with a green leather inset and banker lamp.
See [art provenance](../../world/art/README.md) for the owner's retail reference
and the separately identified similar reference photo. These are original
procedural models using existing CC0 textures/books, with no retailer images
embedded or redistributed and no new dependencies.

Across all six new rooms, chairs, plants, bookcases, worktables, map cabinet,
armillary instruments, sculpture stands and parts bins use the same shared
left-drag, wheel-depth, both-buttons/wheel rotation, WASD carrying, cancellation,
release/gravity and walking-push controls as the study's movable furniture.
Books and shelf ornaments move with each bookcase; table lamps/contents and
sculptures move with their supporting assemblies. Fixed architecture, mounted
art/toolboards/shelves/lights and the garden fountain remain fixed. The original
study desk and its accessories remain fixed as previously requested.

`pushable_furniture.gd` now accepts authored assemblies and fitted convex shapes.
Scaled plants scale their pot collider and visual child while keeping the rigid
body at unit scale. Moving assemblies contain no nested static bodies. Visual
bounds are measured after all contents are attached, in the furniture's local
coordinates, so shared global drag/cancel logic works under rotated room roots.
Application support probes follow actual furniture collisions; moving a table
out from under an application makes it fall and it remains a real usable client.
All placements remain **session-local**; restart restoration is not implemented.

The repeated book sets are combined once by their two original materials and
shared across shelves. Bookcase mouldings/rods/collars are combined by material
with their shadow settings preserved. Geometry, UVs, materials, shadows and
individual movable bays remain intact. No low-detail replacement of the books
or room lighting was used. Small gaps between crowns accommodate drag clearance.

## Verification and evidence

Agent-run and agent-inspected, 2026-10-08–09, `58d2be5` plus T62. Godot 4.7.2
ed1daf0bf, Compatibility/OpenGL 4.6, Mesa 26.2.4, AMD Custom GPU 0405
(Steam Deck), 1280×800 private GPU Weston display, existing 4× MSAA and 60 FPS
cap. The resize check uses 1600×1000. Human physical-input/comfort observation:
**not observed**. No human gate is ticked.

The new `room_furniture.gd` trial exercises eight furniture categories under
rotated parents: actual picking, dragging, carrying while walking, wheel rotation,
Escape restoring the global pose, release/landing and actual walking contact.
It checks scaled pot dimensions, assembly contents following movement, absence
of nested static colliders, and the batched book materials. It also pulls a
bookcase from its actual library row, carries a real application through the
extended halves of all four rooms, verifies unchanged room 05/06 dimensions,
and verifies table support/removal followed by real shell typing.

The first clear-area fixture accidentally approached the observatory door and
instrument; the fixture was moved to an open lateral area without changing any
assertions or product collision rules. The actual-row test then caught too-tight
bookcase spacing, which was fixed. The existing six-room traversal test caught
atlas/gallery props obstructing carried panels; moving those props toward the
side walls fixed the route rather than reducing the required carrying distance.
The hallway extreme-bound check now uses the longer exterior and additionally
requires each rotated corner to lie within an actual room/hall placement volume.

Import/runtime/native-helper validation passes. All 18 Godot checks and 7 Python
checks have passing results across bounded runs; this is not a claim that the
full suite passed in one invocation. The initial full run passed the three
mapping trials, controller, original furniture and hallway checks, then caught
the atlas/gallery clearance defects. After fixing their placement, the six-room
trial and the new room-furniture trial passed, followed by room details, screen
color, live terminal, typing, pointer, resize, launcher and application objects.
The launcher used a bounded standalone allowance (its existing full-suite limit
remains unchanged). No C++ source changed; the native helper was built by the
Godot validator and no new Meson acceptance is inferred.

The transition trial then failed its final client-close assertion once. Code
inspection found no changed close/transition logic in this task; a standalone
recheck on 2026-10-09 passed without changing the test or product. The original
failure remains in `application_transition-final.log`, the passing recheck in
`application_transition.log`; the intermittent cause is **not resolved**. The
four-support application-gravity trial also passed on 2026-10-09. This bounded
room task does not close the broader client-lifecycle gate T45.

Logs: `.tools/t62/suite.log`, `/tmp/elsewhere-study-check.pEqPqA`,
`.tools/t62/remaining.log`, per-trial `*-final.log`, `transition-recheck.log`,
`gravity-run.log` and `final-import.log`. The original failures, fixes and recheck
are retained locally rather than silently overwritten.

Agent-inspected runtime captures:
[library](../evidence/t62/library.png), [atlas](../evidence/t62/atlas.png),
[garden](../evidence/t62/garden.png), [gallery](../evidence/t62/gallery.png),
[individual bookcase pulled into the aisle](../evidence/t62/antique-bookcase-carried.png),
[application in the extended garden](../evidence/t62/garden-extended.png),
[live application supported by a movable table](../evidence/t62/live-table-support.png),
[application on the floor after removing its table](../evidence/t62/removed-table-support.png).
These are actual GPU output, not concept renders or screenshot-replayed clients.
The long room views retain distinct palettes, architectural rhythms and work
areas; the bookcase close-up shows wood grain, books, framing and metal hardware.
The support sequence shows the lamp moving with the table while the application
falls independently. Hard shadow edges remain visible in this renderer.

Reproduction (substitute another test name as needed):

```sh
SDL_JOYSTICK_LINUX_CLASSIC=1 tools/validate-godot-project.sh
SDL_JOYSTICK_LINUX_CLASSIC=1 \
ELSEWHERE_LAUNCHER_STATE_DIR="$PWD/.tools/t62/launcher-state" \
timeout -k 2s 180s weston --backend=headless --fake-seat --renderer=gl \
  --width=1280 --height=800 --socket=elsewhere-t62 --no-config --idle-time=0 \
  --log="$PWD/.tools/t62/weston.log" -- \
  /usr/bin/env WAYLAND_DISPLAY=elsewhere-t62 \
  "$PWD/tools/Godot_v4.7.2-stable_linux.x86_64" --display-driver wayland \
  --path "$PWD/world" --audio-driver Dummy --max-fps 60 \
  --script res://tests/room_furniture.gd
```

The trial must print `ROOM_FURNITURE_OK types=8 extended=4 failures=0` with no
Godot errors; Weston's process exit alone does not report child assertion failures.
The repository's `check-godot-study.sh` now includes this trial with a 180-second
allowance for eight physics interactions and four extended-room routes; existing
assertions and deadlines are unchanged. Local GPU drivers and detailed logs are
under `.tools/t62/`. Generated caches and downloaded source models remain ignored.

For performance, run `elsewhere_rooms.gd -- --rooms-profile` with the same display
recipe. It samples 120 process-frame intervals per settled view, with a real
terminal running and a 60 FPS cap. Final sample (2026-10-09),
`.tools/t62/final-profile.log`:

| View | Median ms | 95th percentile ms | Maximum ms |
| --- | ---: | ---: | ---: |
| Library | 16.672 | 27.077 | 31.064 |
| Atlas | 16.674 | 16.843 | 24.681 |
| Garden | 16.668 | 21.869 | 28.377 |
| Gallery | 16.667 | 16.849 | 20.171 |
| Workshop | 16.667 | 16.906 | 24.678 |
| Observatory | 16.664 | 16.859 | 24.077 |

Godot reported approximately 104MiB of static allocations; this excludes GPU
memory and is not total process RSS. Before batching, library/garden medians
were 43.020/39.296ms (`profile-unbatched.log`); combining the imported books and
then the authored joinery removed repeated draw calls while keeping the same
visible geometry/materials and shadowing. These stationary private-compositor
samples do not measure input latency, prolonged traversal or human comfort.
The broader performance/art gates remain separate. Final import/runtime logs:
`/tmp/elsewhere-godot-check.0CYuRc`. Documentation links, the unique added T62 ID,
shell syntax and diffs were checked. Existing historical task-ID references were
left intact. Nothing was pushed; restart restoration and T45 remain out of scope.
