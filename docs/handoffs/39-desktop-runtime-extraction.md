# P22-T01 — Desktop service extraction

Owner-requested single task, 2026-10-09, base `fd36f01`. Decision 08 governs.
The active environment remains the furnished study and connected rooms.

## Dependency audit and change

Before extraction, `study.gd` constructed the room, monitor mesh/status label,
lighting and HUD, instantiated all desktop services, wired their references,
disabled automatic quit and handled window-close shutdown. `main.tscn` provided
only the study root and player/camera. The actual responsibilities already lived
in separate scripts:

- `terminal_screen.gd`: native `ElsewhereCompositorSession`, pump, initial Weston
  terminal, launcher-owned child PIDs, selected-window snapshots/texture, monitor
  presentation and graceful/forced cleanup.
- `application_mode.gd`: keyboard/pointer focus, ordinary application input,
  reserved return shortcut, flat view, resize and host-focus cleanup.
- `app_launcher.gd` and `world/tools/desktop-apps.py`: catalog, recents, launch
  recipes, owned launch processes and temporary window associations.
- `application_objects.gd` / `application_object.gd`: live panels, independent
  entity IDs, transient bindings, picking, drag/carry/rotation and gravity.
  The manager directly imported the study hallway's placement limits.
- `application_transition.gd`: the live 220ms presentation proxy.
- `game_world.gd`: player movement, mouse capture, collision and study spawn.

`desktop_runtime.gd` now owns service composition, HUD and window-close policy.
It is a plain Node in `main.tscn`, so it introduces no additional spatial
transform. The study makes one `initialize(player, screen, status, bounds)` call
after building its physical presentation. The existing services remain its
children in the same relative order. Native resource/process ownership stays
in the terminal child; no new registry, session, thread, compositor interface,
dependency or persistent identity was introduced.

Placement receives a callable taking world-space center and half-extents and
returning a valid center. The study passes the unchanged hallway policy; the
manager no longer imports room construction. Read-only study accessors preserve
the existing test API without duplicate ownership. Physical scene paths, geometry,
lighting, materials, controls, client launch recipes and default environment
are preserved.

The new headless `desktop_runtime.gd` test uses real Weston terminals and generic
presentation nodes without loading study geometry/assets. It checks mapping,
live panel creation, rotated placement against a different supplied boundary,
independent entity identity, repeated orderly shutdown, forced service removal,
child-process/socket cleanup and preservation of the host display environment.
The fixture is not a new location or an outdoor startup scene.

## Reproduction and evidence

Commands run from the repository root. Logs/capture copies are under ignored
`.tools/p22-t01/before/` and `.tools/p22-t01/after/`.

Before editing existing implementation, the following passed against `fd36f01`:

```sh
GODOT_BINARY="$PWD/.tools/rename/godot-check.sh" tools/validate-godot-project.sh
SDL_JOYSTICK_LINUX_CLASSIC=1 tools/check-godot-binding.sh
for trial in application_objects terminal_pointer application_transition; do
  ELSEWHERE_LAUNCHER_STATE_DIR="$PWD/.tools/p22-t01/before/launcher-state" \
    timeout -k 2s 90s .tools/rename/godot-check.sh --path "$PWD/world" \
    --audio-driver Dummy --max-fps 60 --script "res://tests/$trial.gd"
done
```

Baseline markers: `APPLICATION_OBJECTS_OK clients=2 failures=0`,
`TERMINAL_POINTER_OK selection_pixels=7496 scroll_pixels=1875 failures=0`,
`APPLICATION_TRANSITION_OK failures=0`. Native binding checks passed class
discovery, 300 probe calls, lifecycle, owned frames, keyboard, pointer, resize
and surface-tree fixtures. Expected invalid-protocol fixture errors are not
application failures.

After extraction:

```sh
GODOT_BINARY="$PWD/.tools/rename/godot-check.sh" tools/check-godot-study.sh
GODOT_BINARY="$PWD/.tools/rename/godot-check.sh" tools/check-chrome-application.sh
meson compile -C build -j 2
meson test -C build --print-errorlogs
node --test .opencode/tests/auto-continue.test.js
```

Post-change results:

- Clean Godot import and default headless startup passed.
- All 21 Godot study trials passed, including the new two-session desktop test,
  movement/focus, all existing room/furniture routes, live Weston terminal
  output, keyboard/selection/scroll, four-size actual client resize, launcher,
  two-client carrying/rotation, transition interruption and gravity/support.
  The suite also passed all 8 Python desktop-entry/launcher tests. Logs:
  `.tools/p22-t01/after/study.log` and `after/trials/`.
- `meson compile -C build -j 2` and all 34 Meson tests passed. C++ source and
  bridge interfaces were unchanged.
- Real Chrome 154.0.8037.57 passed `CHROME_APPLICATION_OK failures=0`: actual
  catalog launch, subsurface suggestions, typing/click/scroll, resize, world
  return, a second window and browser-initiated close of one window. Log:
  `.tools/p22-t01/after/chrome.log`. The
  [typed/clicked browser view](../evidence/p22-t01/after-browser.png) and room
  return capture were inspected; remaining captures are in `after/chrome-captures/`.
- All 62 Node plugin/parser tests passed. The numeric-ID scan finds 148 unique
  numeric task IDs and all ten P22 tasks. Historical P1 letter-suffixed IDs are
  excluded from that numeric scan. Shell syntax, changed-document relative
  links and whitespace/diff checks passed.

Agent-reviewed actual GPU renders, 1280×800:

- [Before panel rotation](../evidence/p22-t01/before-rotation.png) and
  [after panel rotation](../evidence/p22-t01/after-rotation.png): the same study,
  furnishings, light/shadow, HUD, separate Vim panel and angled live terminal.
  The terminal clock continues changing; panel placement/orientation assertions
  are checked by the real-client trial, not inferred from the still images.
- [Before selection](../evidence/p22-t01/before-selection.png) and
  [after selection](../evidence/p22-t01/after-selection.png): readable native
  application view and selected terminal text. These independently generated
  captures are byte-identical (SHA-256
  `132d93b397fd5ba68c798b3d8b4972b4cce4d0cc92f7bd4eb7ed5b7b79baf7c1`).
  An initially incomplete image preview was resolved by reopening the original
  PNG; the stored render itself was correct.
- [Typed terminal](../evidence/p22-t01/after-typing.png): actual shell output,
  interrupted command and subsequent input after returning from world mode.
  The transition midpoint and application endpoint were also inspected in
  `.tools/p22-t01/after/application-transition-test/`.

The existing private GPU Weston wrapper is `.tools/rename/godot-check.sh`
(introduced during T67). It exports `SDL_JOYSTICK_LINUX_CLASSIC=1`, directly
executes the repository Godot for `--headless`, and otherwise runs:

```sh
# Set width=1600 and height=1000 for terminal_resize.gd; otherwise 1280x800.
weston --backend=headless --fake-seat --renderer=gl \
  --width="$width" --height="$height" --socket="elsewhere-rename-check-$$" \
  --no-config --idle-time=0 --log="$PWD/.tools/rename/weston-check-$$.log" -- \
  /usr/bin/env WAYLAND_DISPLAY="elsewhere-rename-check-$$" \
  "$PWD/tools/Godot_v4.7.2-stable_linux.x86_64" --display-driver wayland "$@"
```

This is a separate virtual output using the actual AMD GPU, not a replacement
host desktop or a headless Godot screenshot. Godot 4.7.2 Compatibility/OpenGL,
Mesa 26.2.4, AMD Custom GPU 0405 (Steam Deck/Vangogh), Weston/Weston terminal
15.0.1 and Vim 9.2 (patches 1–1046). The normal before/after
user launch command remains `tools/run-godot.sh --audio-driver Dummy`.

## Limits and stopping point

This boundary supports initialization in another location, not hot-swapping
physical scenes while retaining an active session. The caller must keep the
player, monitor mesh and status label alive for the desktop lifetime. The fixed
monitor aperture, Filmic screen compensation and study spawn defaults remain
existing presentation contracts. No restart persistence, new application
protocols, native-session installation, or broad usability gate is claimed.
Popup menus, clipboard and GPU client buffers retain their recorded limitations.
Physical-input feel and owner comfort are not observed by these injected tests.

P22-T01 is complete. No required check was unavailable. Human physical-input and
comfort checks were not performed and remain unobserved. P22-T02 and all other
tasks remain unstarted and require a separate explicit owner request.
