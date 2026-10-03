# Godot study frontend

The study renders six imported Poly Haven furniture models, a textured room,
lighting and a collision-based first-person controller. The monitor displays
live output from a real Weston terminal through the retained C++ Wayland core.
Keyboard/pointer delivery and application mode are not implemented yet.

From the repository root, using the existing local tool/cache:

```sh
python3 world/tools/prepare_study_assets.py
tools/validate-godot-project.sh
tools/run-godot.sh --audio-driver Dummy
```

The preparation step verifies model/dependency SHA-256 values and copies complete
source glTF packages into the import directory. Missing or corrupt dependencies
fail explicitly. It does not download missing assets. Fresh-machine tool and
asset bootstrap remains unfinished; see [status](../docs/STATUS.md).

`tools/run-godot.sh --editor` opens the editor. The default command runs the
study. `build/mansion-desktop --room-camera` is the separate legacy renderer.
Godot uses the Compatibility/OpenGL renderer, not Vulkan.

The launcher builds and stages the recovered standard GDExtension before opening
Godot. It needs the local godot-cpp checkout and existing C++ build dependencies.
`/usr/bin/weston-terminal` (tested: Weston 15.0.1) starts `/bin/sh` on the private
socket, using isolated configuration. Missing terminals produce an explicit
error; there is no simulated fallback. For visibly changing output, run:

```sh
tools/run-godot.sh --audio-driver Dummy -- --terminal-demo
```

This runs an ordinary clock/counter program inside the real terminal. Closing
the Godot window stops its server and owned terminal; host display settings stay
unchanged. `-- --no-terminal` disables launch for isolated scene checks.

| Control | Action |
| --- | --- |
| W/A/S/D | Walk, relative to camera yaw |
| Hold right mouse | Look; release restores the pointer |
| Escape | Release pointer and clear held movement |
| Home | Return to the safe arrival position |
| M | Toggle slow walking |
| Shift | Walk slowly while held |

There is no camera bob. Losing window focus clears held input and releases the
pointer. Close using the host window close control.

## Verification

```sh
# Import and runtime errors must both fail this command.
tools/validate-godot-project.sh
# Requires a display: controller, screen colors and two real terminal sessions.
tools/check-godot-study.sh
# Three actual GPU-rendered views. Use an absolute output directory.
tools/run-godot.sh --audio-driver Dummy -- --capture=/tmp/mansion-study
```

The smoke test injects input into the controller; it is not a human usability
trial. Headless display drivers cannot verify pointer capture. Visual acceptance
requires opening the captured images, not merely creating them.

The old native bridge is preserved under `addons/mansion_godot/`, excluded from
Godot discovery by `.gdignore` because initialization crashes. The recovered
standard binding is staged under `addons/mansion_runtime/`; its separate
protocol/byte-level regression command is `tools/check-godot-binding.sh`.
See [live output evidence and limitations](../docs/handoffs/20-godot-live-terminal.md).
