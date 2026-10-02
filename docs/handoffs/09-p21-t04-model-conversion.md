# Handoff: P21-T04 — Convert models and verify PBR imports

**Date:** 2026-10-01
**Status:** Blocked — requires Godot editor (display server or local install)
**Depends on:** P21-T01 (scaffold), P21-T03 (assets cached)

## What is done

- **P21-T01 scaffold:** `world/scenes/main.tscn` has room geometry (floor, 4 walls,
  ceiling), monitor slot with emissive screen, desk placeholder, Camera3D, SpawnMarker.
  `world/project.godot` uses Compatibility (Vulkan) renderer, 1280×800.
- **P21-T03 assets:** 22 primary assets cached in `world/assets/cache/` (2.8 GB):
  - 1 HDRI: `poly_haven_studio` (24K EXR, 2.7 GB)
  - 11 textures: `walnut_veneer`, `beige_wall_001`, `white_plaster_02`,
    `curly_teddy_natural`, `brown_leather` (diffuse/normal/rough/AO per texture)
  - 10 models: `metal_office_desk`, `dining_chair_02`, `wooden_bookshelf_worn`,
    `desk_lamp_arm_01`, `potted_plant_02`, `book_encyclopedia_set_01` (+ alts)
  - Models downloaded as GLTF from Poly Haven API (no direct GLB available)
- **Manifest:** `world/assets/manifest.md` documents all 30 selected assets with
  IDs, resolutions, texture maps, and material assignment plan.

## What needs to be done (P21-T04 acceptance)

1. **Convert model formats where required.** Poly Haven provides models as GLTF,
   not GLB. Decide whether to:
   - Convert GLTF → GLB using Blender (headless: `blender --background --python convert.py`)
   - Import GLTF directly in Godot 4.x (Godot 4 supports GLTF natively)
   - Use Blender to convert and verify PBR materials in one step
   Record the exact pinned tool version and command.

2. **Deliberate Godot import and material settings.** Import cached models into the
   Godot scene with explicit material settings:
   - Set texture import flags (sRGB for albedo, non-sRGB for roughness/normal/AO)
   - Set normal map convention (OpenGL vs DirectX)
   - Set compression settings (ASTC for Steam Deck target)
   - Verify texture channel mapping: albedo→base_color, normal→normal, roughness→roughness, AO→ambient_occlusion

3. **Render a model/material contact sheet.** Create a test scene with each imported
   model or material sample under the project HDRI (`poly_haven_studio`). Render
   and inspect:
   - Texture channel correctness (no swapped normal channels, correct roughness values)
   - Scale (models should be in correct real-world dimensions from Poly Haven metadata)
   - Normal orientation (bump direction should be physically correct)
   - Provenance (each material should reference the correct Poly Haven asset ID)

4. **Retain exact commands and pinned versions.** Document:
   - Blender version used for conversion (if used)
   - Godot import settings screenshots or .import file contents
   - Render commands/settings used for contact sheet

## Blocked on

Display server availability or local Godot/Blender install. The development container
has neither a display server nor installed Godot/Blender binaries (they are
repository-local under `tools/` but require X11/Wayland to run).

**Missing in container:** wlroots, weston, Xvfb, eglinfo, wayland-info.
`/dev/dri/renderD128` is accessible but no display server is running.
Installing packages requires `sudo` (autonomous sessions must not use).

## Continuation steps (when display server available)

1. Verify Godot editor can start: `./tools/godot-4.7.2-stable/bin/godot.linuxbsd.editor.dev.x86_64 --path world/`
2. Open the project, import a model from `world/assets/cache/`
3. Set import flags, verify PBR channels, render contact sheet
4. Render and inspect the material contact sheet
5. Update `world/scenes/main.tscn` with converted models
6. Run C++ regression suite to confirm no regressions: `meson test -C build`
7. Tick P21-T04 in TASKS.md, update STATUS.md, commit

## Related files

- `world/assets/manifest.md` — asset selection and material assignment plan
- `world/assets/cache/` — cached Poly Haven assets (2.8 GB)
- `world/scenes/main.tscn` — scaffold scene (needs model import)
- `world/project.godot` — project config (Compatibility renderer, 1280×800)
- `world/tools/fetch_assets.py` — asset fetcher tool
- `tools/godot-4.7.2-stable/bin/godot.linuxbsd.editor.dev.x86_64` — Godot editor binary (1.07 GB)
- `docs/ART-DIRECTION.md` — art direction for the study room
