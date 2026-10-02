# Mansion Desktop — Godot world frontend

## Quick start

```sh
# Validate the project scaffold (no display required)
tools/validate-godot-project.sh

# Open the Godot editor
tools/run-godot.sh

# Run headless (for CI / scripted testing)
tools/run-godot.sh --headless --quit
```

## Project structure

```
world/
├── project.godot          # Godot project config (Compatibility/Vulkan renderer)
├── scenes/
│   └── main.tscn          # Main scene: room, monitor slot, spawn point
└── scripts/
    └── game_world.gd      # Camera movement, collision, input modes, teleport
```

## Run commands

| Command | Purpose |
|---|---|
| `tools/run-godot.sh` | Open Godot editor with the world project |
| `tools/run-godot.sh --headless --quit` | Validate project loads without GUI |
| `tools/validate-godot-project.sh` | Automated validation (scene nodes, scripts, import) |
| `GODOT_BINARY=<path> tools/run-godot.sh` | Use a specific Godot binary |

## Import commands

```sh
# Re-import the project (refreshes .godot folder and cached resources)
tools/Godot_v4.7.2-stable_linux.x86_64 --headless --import world/

# Or from the Godot editor:
#   File → Project → Clear Project Cache → Reimport All
```

## Input (P21-T01 scaffold)

| Key | Action |
|---|---|
| W/A/S/D, Arrow keys | Move camera |
| Right mouse button | Look around (capture mode) |
| T | Teleport to monitor slot |
| Enter | Enter application mode |
| F12 | Return to world mode from application mode |

## P21-T01 acceptance

- ✅ `world/project.godot` — project config with Compatibility renderer
- ✅ `world/scenes/main.tscn` — main scene with room geometry, monitor slot, spawn point
- ✅ `world/scripts/game_world.gd` — camera controller with collision bounds and input modes
- ✅ `tools/validate-godot-project.sh` — automated import and scene validation
- ✅ `tools/run-godot.sh` — documented run command
- ✅ C++ headless tests preserved (28/28 pass, unchanged)
- ⚠️  Live render capture — requires a display server; not possible in headless container.
     Rendered scene will be captured once P21-T07 (movement) and P21-T09 (visual review)
     are complete.

## Next tasks

- **P21-T02**: Curate Poly Haven asset set (independent, no Godot needed)
- **P21-T07**: Implement comfortable world movement (depends on P21-T01)
- **P21-T11**: Build GDExtension adapter (depends on P21-T01)
