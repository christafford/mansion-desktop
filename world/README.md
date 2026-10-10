# Godot world frontend

The default scene is the **P22-T02 minimal outdoor startup**: flat walkable
24m ground, visible solid boundaries, a safe spawn, sky and sunlight. It reuses
the player controller and desktop runtime. The existing real Weston terminal
opens as a live world panel. This is a startup foundation, not the authored
village, polished art or the P22-T06 outdoor application-integration gate.

From the repository root, using the existing pinned tools:

```sh
tools/validate-godot-project.sh
tools/run-godot.sh --audio-driver Dummy
```

No study assets are needed by the default scene. The launcher builds and stages
`addons/elsewhere_runtime/` using the repository's godot-cpp checkout and C++
build dependencies. Godot uses Compatibility/OpenGL. `/usr/bin/weston-terminal`
starts `/bin/sh` on the private compositor socket, with isolated configuration,
prompt and repository-local shell history. A missing terminal is an explicit
error, with no simulated fallback. Fresh-machine bootstrap remains unfinished.

`tools/run-godot.sh --editor` opens the editor. `build/elsewhere --room-camera`
remains a separate legacy renderer. For changing real terminal output:

```sh
tools/run-godot.sh --audio-driver Dummy -- --terminal-demo
```

Closing the Godot window stops its server and owned applications. Host desktop
settings stay unchanged. `-- --no-terminal` disables the initial terminal for
isolated scene checks; the application-status label reports this in the world.

| Control | Action |
| --- | --- |
| W/A/S/D | Walk relative to heading; A/D strafe |
| Hold right mouse | Look; release frees the pointer |
| Shift | Walk faster |
| Ctrl / M | Hold / toggle slow walking |
| Space | Jump while grounded |
| Escape | Release pointer, cancel a grab and stop movement |
| Home | Return to this scene's safe arrival pose |
| Enter / double-click a panel | Activate the selected application |
| Ctrl+Alt+Escape | Return from application mode to the world |
| Tab / Applications button | Open the application launcher in world mode |
| Left-drag a panel | Move/carry it; WASD and mouse-look remain available |
| Wheel while dragging | Adjust distance |
| Both buttons + wheel | Rotate the held panel |

Application mode uses physical US keyboard positions. Ordinary application
keys, including Space, Escape, Tab, Home and WASD, belong to the client. Pointer
selection/scroll and actual client resize retain their existing paths. Host
focus loss returns to world mode and releases held input. Type `exit` to close
the shell and use Applications to relaunch. Clipboard, popup menus, IME and GPU
client-buffer support retain their documented limitations. Placements are
session-local; restart restoration is not implemented.

## Retained study and tests

The old physical world is preserved as `scenes/study.tscn`, with its scripts and
assets unchanged. It is loaded only explicitly, including by study-specific
regressions. To run it using the existing asset cache:

```sh
python3 world/tools/prepare_study_assets.py
tools/run-godot.sh --audio-driver Dummy res://scenes/study.tscn
# The study's historical capture command remains available explicitly.
tools/run-godot.sh --audio-driver Dummy res://scenes/study.tscn -- --capture=/tmp/elsewhere-study
```

Asset preparation verifies manifest SHA-256 values and stages complete glTF
packages; it does not fetch missing assets. Do not discard those sources/cache.

```sh
# Headless and graphical default startup, movement/jump/collision, live terminal.
tools/check-godot-outdoor.sh
# Retained study geometry plus general desktop checks on the current default.
tools/check-godot-study.sh
# Existing bounded real-Chrome compatibility regression on the current default.
tools/check-chrome-application.sh
```

The outdoor check writes actual GPU captures to `.tools/outdoor-startup-test/`.
Open them for visual review. Headless checks cannot supply visual acceptance;
injected input and screenshots do not establish human comfort or comprehensive
application compatibility. See [status](../docs/STATUS.md) and the
[P22-T02 handoff](../docs/handoffs/40-outdoor-startup.md) for exact evidence.

The old experimental bridge remains under `addons/elsewhere_godot/`, excluded
from discovery by `.gdignore`. The standard binding's protocol/owned-frame
regression command remains `tools/check-godot-binding.sh`.
