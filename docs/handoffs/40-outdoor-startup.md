# P22-T02 — Minimal outdoor startup

Owner-requested single task, 2026-10-10, base `7d74847` (P22-T01 plus the
owner's jump commit). Authority: Decision 08. P22-T03 is not started.

## Change and preserved boundaries

`world/scenes/main.tscn` now contains a plain outdoor startup area: 24m ground,
four visible 1.4m collision boundaries, procedural sky, ambient light and a
shadowed sun. The ground extends under the boundaries so there are no edge
gaps. Spawn is `(0, 0.05, 5)`, facing along -Z; the capsule settles onto y=0.
The boundary height contains the owner's existing jump from the ground.
This is deliberately a small primitive startup fixture, not the 70–100m village
landscape, circulation, architecture or art acceptance of later P22 tasks.

`outdoor.gd` makes the existing DesktopRuntime initialization call with its
player, presentation nodes and inset placement bounds. It loads no study or
hallway code/assets. The selected-preview mesh has a hidden parent, preserving
the existing selected texture/local visibility interface without adding a
physical monitor. Its shader uses the existing material factory. Independent
live application panels remain visible; a separate world status label still
reports startup/disabled/error states. The environment retains the exact Filmic
exposure/white contract used by the client screen shader.

The only controller change is exported spawn position/yaw/pitch used by startup
and Home. Its defaults retain the historical study pose. The owner's Space jump,
gravity, speeds, input, collision and focus logic are untouched. Desktop runtime,
terminal/session owner, launcher, application modes/objects/transitions, native
bridge, client launch recipes and stable-identity behavior are unchanged.

The original `main.tscn` is preserved byte-for-byte as `scenes/study.tscn`
(checked against `git show 7d74847:world/scenes/main.tscn`). No legacy geometry,
scripts, source assets, caches or tests were deleted. `project.godot` retains
its main-scene path and all `elsewhere` names/entry points remain intact.

## Deliberate test migration

Eleven tests containing study-specific geometry, camera views or placement
coordinates now load `res://scenes/study.tscn`; only that path changes:
`study_smoke`, `furniture_drag`, `hallway`, `elsewhere_rooms`, `merged_rooms`,
`room_furniture`, `grand_foyer`, `study_details`, `live_terminal`,
`application_objects`, `application_gravity`. Their existing assertions remain.

The existing general `terminal_input`, `terminal_pointer`, `terminal_resize`,
`app_launcher`, `application_transition` and separate `chrome_application`
trials still load `main.tscn`, so they now exercise the actual outdoor default.
The Chrome fixture/check now additionally waits for document load and input
focus before typing; its original interaction assertions remain unchanged.
`check-godot-study.sh` retains its entry point and all 21 trials; it is explicitly
a mixture of retained geometry regressions and current-default desktop checks.
Its name is not a claim that all trials now run in the old world.

The new `outdoor_startup.gd` resolves `application/run/main_scene`, checks that
no study/hallway/rooms/foyer scripts were loaded, and verifies sky/light, real
terminal mapping, floor support, walking/release, the owner's jump/landing,
walking/jumping against all four boundaries, diagonal corner containment,
configured Home and clean shutdown. In graphical mode it also exercises one
live panel's drag/rotation/cancellation, activation, shell-written marker and
return to the same instance, and records actual GPU renders. This bounded
startup regression does not replace P22-T06 or approve future village geometry.

## Commands and evidence

Run from the repository root with the existing pinned tools and installed
Weston terminal. The private GPU Weston wrapper from P22-T01 remains available
at `.tools/rename/godot-check.sh`; its command/settings are documented in
[handoff 39](39-desktop-runtime-extraction.md). It passes headless Godot calls
straight through and uses a separate GPU-rendered Weston virtual output for
graphical tests. It changes no host desktop or services.

```sh
GODOT_BINARY="$PWD/.tools/rename/godot-check.sh" tools/check-godot-outdoor.sh
GODOT_BINARY="$PWD/.tools/rename/godot-check.sh" tools/check-godot-study.sh
GODOT_BINARY="$PWD/.tools/rename/godot-check.sh" tools/check-chrome-application.sh
GODOT_BINARY="$PWD/.tools/rename/godot-check.sh" tools/run-godot.sh \
  --headless --audio-driver Dummy --max-fps 60 --quit-after 30 -- --no-terminal
node --test .opencode/tests/auto-continue.test.js
```

Logs are retained in ignored `.tools/p22-t02/`. Godot import/default headless
startup and explicit `--no-terminal` startup pass. The extension build/stage
runs through the normal launcher/validation helper; no native source changed.
The two outdoor modes pass (`OUTDOOR_STARTUP_OK`, four boundaries, zero failures),
with detailed logs in `/tmp/elsewhere-outdoor-check.9y9QdV`. The existing Node
plugin/parser suite passes all 62 tests. Shell syntax and retained-scene byte
identity checks pass.

All 21 existing Godot regression trials pass, including real terminal typing,
selection/scroll, resizing, relaunch, multiple live panels, transitions, placement
and gravity. All eight Python desktop-catalog/launcher tests pass. Detailed
trial logs are copied from `/tmp/elsewhere-study-check.JdLkBU` into
`.tools/p22-t02/regression-details/`; the two outdoor logs are also copied into
`.tools/p22-t02/outdoor-details/`. The existing selection capture was inspected:
selected text is upright and readable. No old assertion was weakened or removed.
Native Meson/binding test results remain historical P22-T01 evidence; they are
not represented as fresh runs for this scene-only task.

The final real Chrome 154.0.8037.57 trial passes (`CHROME_APPLICATION_OK
failures=0`): address entry, trusted typing/click, scroll, resize/reflow, world
return, second-window identity and client-initiated close. See
`.tools/p22-t02/chrome.log` and the initial-attempt explanation below.
Documentation links, unique numeric task IDs, actual P22 parser output and
`git diff --check` also pass.

The first outdoor-test attempt compared the exact 5cm spawn clearance after
awaiting frames; physics had already settled it onto the floor. The new test
now checks the reset immediately (as the existing controller trial does) and
separately exercises grounded movement. No controller behavior was changed to
accommodate that test-timing error. Both final outdoor modes pass.

The first Chrome trial lost the initial `E` in its note. Inspection found its
empty-value readiness check could accept a resize report before the fixture's
`onload` handler focuses the input. The missing first character is consistent
with that readiness race.
The fixture now reports document-complete plus focused-input readiness, and the
trial requires that before typing. This adds synchronization without changing
runtime input or any expected interaction result. The original failure log and
captures are retained under `.tools/p22-t02/chrome-initial*`.

Agent-inspected Godot Compatibility/OpenGL captures (1280×800 unless noted):

- [Arrival](../evidence/p22-t02/arrival.png): sky, flat ground, solid visible
  boundary, sunlight/shadows and one real terminal panel; no active study.
- [Ground and boundary](../evidence/p22-t02/ground-and-boundary.png): the plain
  walkable area from a second eye-level position, with the same live panel.
- [Application](../evidence/p22-t02/application.png): readable native-size
  shell command/output (`OUTDOOR_OK`) delivered through the retained compositor.
- [Chrome input](../evidence/p22-t02/browser-input.png): complete typed note
  and clicked Save result remain upright and readable.
- [Chrome in the world](../evidence/p22-t02/browser-in-room.png) (1000×700):
  the same running browser returns to its live panel in the outdoor scene.

Observer: Codex agent. Platform: Godot 4.7.2, Mesa 26.2.4, AMD Custom GPU 0405
(Steam Deck/Vangogh), private Weston 15.0.1. These renders demonstrate only the
minimal startup and live content. Flat materials, box boundaries and an empty
sky horizon are intentional scope limits; there is no village-art approval.

## Run and stop

Default: `tools/run-godot.sh --audio-driver Dummy`. No study assets are needed by
this scene. Existing nested/headless paths and `--terminal-demo` / `--no-terminal`
remain available. To revisit the preserved environment explicitly:

```sh
python3 world/tools/prepare_study_assets.py
tools/run-godot.sh --audio-driver Dummy res://scenes/study.tscn
```

The normal default uses the same real-client launcher and desktop services,
but comprehensive outdoor application integration belongs to P22-T06 after
the authored village and owner art checkpoint. Restart persistence, scene
streaming, popup/clipboard/GPU-buffer compatibility and human comfort remain
outside this task. No human task is ticked. Stop after P22-T02; P22-T03 and any
other work require a separate explicit owner request.
