# Godot study frontend

The study renders six imported Poly Haven furniture models, a textured room,
lighting and a collision-based first-person controller. The monitor explicitly
shows an inactive preview; it is **not a live terminal** yet.

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
# Requires a display: tests real mouse capture, movement and wall collision.
tools/run-godot.sh --audio-driver Dummy --script res://tests/study_smoke.gd
# Three actual GPU-rendered views. Use an absolute output directory.
tools/run-godot.sh --audio-driver Dummy -- --capture=/tmp/mansion-study
```

The smoke test injects input into the controller; it is not a human usability
trial. Headless display drivers cannot verify pointer capture. Visual acceptance
requires opening the captured images, not merely creating them.

The old native bridge is preserved under `addons/mansion_godot/`, excluded from
Godot discovery by `.gdignore` because initialization crashes. Before reenabling
it, prove the replacement binding loads, is callable and shuts down cleanly in
an isolated project. Never remove the ignore file just to claim integration.
