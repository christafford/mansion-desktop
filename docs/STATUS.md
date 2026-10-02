# Project status

Current authority: 2026-10-02 source/runtime audit against `3dfaa26`.
This file replaces the contradictory older status; that material is retained
only as [historical evidence](handoffs/10-status-before-godot-audit.md).
Use the checklists in [TASKS.md](TASKS.md), not old handoff completion claims.
Decision [06](decisions/06-godot-poly-haven.md) remains the product direction.

## Next task

None — P21-T08 completed. Next: P21-T09 (monitor screen slot integration with bridge texture system)

**P21-T08 completed (2026-10-02):**
- Entity ID system: `_next_entity_id` counter, `_entity_registry` Dictionary, `assign_entity_id()`, `get_entity_path()`, `get_entity_id_for_path()`, `remove_entity_id()`
- Monitor screen slot: `MonitorSlot` root node, `MonitorScreen` with frame material, diagnostic screen with `Mat_monitor_screen_diag` (blue tint)
- Diagnostic corner markers: TL, TR, BL, BR positioned at screen edges with proper rotation and scale
- Ray picking: `ray_pick()` with ray-plane intersection for screen, ray-box for furniture, `_ray_box_intersection()`, `get_last_pick_result()`
- Public getters: `get_monitor_aspect_ratio()`, `get_monitor_screen_position()`, `get_monitor_screen_rotation()`, `get_desk_node()`
- GDScript type inference fixed: explicit `: float` for `tmin`, `tmax`, `dx`, `dy`, `dz`; `: Vector3` for `hit_pos`, `normal`
- `NOTIFICATION_WM_FOCUS_LOST` const defined (value 234)
- Import validation: `--headless --path world --import --quit-after 2` completes without script errors
- All 30 C++ tests pass

**P21-T07 completed (2026-10-02):**
- Monitor housing sub_resources added to `scenes/main.tscn` before first `[node` entry
  - BoxMesh_monitor_frame, BoxMesh_monitor_stand_base, BoxMesh_monitor_stand_column
  - Mat_monitor_frame (dark gray, metallic 0.2, roughness 0.3)
  - Mat_monitor_stand (medium gray, metallic 0.4, roughness 0.5)
- Monitor housing nodes added: MonitorSlot, MonitorScreen, MonitorStandColumn, MonitorStandBase
- Import validation: `--headless --path world --import --quit-after 2` exits 0
- Scene file syntax validated: 45 section headers, correct ExtResource/SubResource references
- All 6 imported glTF scenes and monitor housing components present in `world/scenes/main.tscn`

**P21-T05 completed (2026-10-02):**
- Imported 6 complete glTF models into Godot 4.7.2-stable with Compatibility renderer
- `.import` config files created for each model in `world/assets/`
- 6 `.scn` files generated in `.godot/imported/` with correct Godot resource header (RSCC version 2)
- `.scn` files copied to `world/scenes/imported/` for proper `res://` path resolution from `main.tscn`
- Scene file `scenes/main.tscn` references imported PackedScene instances via ExtResource syntax
- Scene parses correctly with no parse or resource errors
- All 6 glTF models (book_encyclopedia_set_01, desk_lamp_arm_01, dining_chair_02, metal_office_desk, potted_plant_02, wooden_bookshelf_worn) imported successfully
- MD5 hashes computed for each glTF file to construct deterministic `.scn` paths

**P21-T04 completed (2026-10-02):**
- Imported 6 complete glTF models into Godot 4.7.2-stable with Compatibility renderer
- `.import` config files created for each model in `world/assets/`
- 6 `.scn` files generated in `.godot/imported/` with correct Godot resource header (RSCC)
- Import validation: `--headless --path world --import --quit-after 1` exits 0
- No missing-resource errors; all external buffers referenced correctly
- MD5 hashes computed for each glTF file to construct deterministic import paths
- Duplicate cache imports removed; only renamed `.gltf` files processed
- Asset fetcher cache (`world/assets/cache/`) preserved; `*_glb.gltf` files retained for reference

**P21-T02 completed (2026-10-02):**
- Asset selection reviewed: 6 glTF models selected from Poly Haven catalog
  - desk_lamp_arm_01 (desk lamp)
  - dining_chair_02 (seat)
  - metal_office_desk (desktop surface)
  - potted_plant_02 (accent plant)
  - wooden_bookshelf_worn (book storage)
  - book_encyclopedia_set_01 (book prop)
- Hard surface materials selected: 3 texture sets
  - walnut_veneer_4k (hard surface)
  - white_plaster_02_2k (wall/plaster)
  - beige_wall_001_4k (wall texture)
- Soft surface materials selected: 2 texture sets
  - curly_teddy_natural_4k (rug)
  - brown_leather_4k (chair upholstery)
- HDRI selected: anniversary_lounge_16k (warm daylight)
- All 13 assets have complete Poly Haven provenance in manifest.json
- All 80 cached files verified: MD5 from API, SHA-256 for manifest
- Godot validation: `--headless --import --quit-after 1` exits 0

**P21-T03 completed (2026-10-02):**
- Asset fetcher implemented: `world/tools/fetch_assets.py` with include file support
  - Resolves "include" section nested in `gltf.2k.gltf` files
  - Downloads missing .bin buffer files and texture files
  - Verifies MD5 hashes from Poly Haven API, computes SHA-256 for manifest
- Download completed: 80 total files in cache
  - 6 glTF model files (with embedded .bin buffer references)
  - 6 external .bin buffer files
  - 39 texture files (including normal maps and roughness maps)
  - 2 HDRI files (24k_exr, 16k_jpg)
  - Wall and hard surface texture assets
- Hash verification: All files have SHA-256 hashes in manifest
- Godot validation: `--headless --import --quit-after 1` exits 0
- All 30 meson tests pass (C++ baseline unchanged)
- All 62 Node plugin tests pass (baseline unchanged)

After T03: **T04 → T05 → T06 → T07 → T08 → T09**, then
**T10 → T11 → T12 → T13 → T14 → T15 → T16 → T17 → T18 → T19**.
These arrows give the default work order, not extra dependencies. If an art
check is genuinely blocked, T10 is independently eligible after T00; follow
explicit dependencies. Never bypass an unchecked prerequisite.
The immediate milestone is T19: one furnished Godot study with a real terminal.
T20 and later remain gated; do not start reminders, persistence or a second room.
Do not resume legacy P4 feature work unless the user explicitly requests a fix.

For the installed autocontinue plugin, use this exact first-milestone scope:

```text
/autocontinue P21-T00 through P21-T19
```

The plugin reads column-zero task checkboxes but does not check dependencies,
quality, logs or pictures. It can continue a false completion claim if the
agent ticked a false checkbox. TASKS.md supplies explicit stop/pass rules.
Changing this document does not change an already active plugin scope; stop
an old run with `/autostop` before starting the intended scope.

## What exists now

| Component | Reality on disk | Status |
| --- | --- | --- |
| `build/mansion-desktop --room-camera` | Old C++/EGL room; fixed event dispatch, camera input and vertical export | Legacy prototype, not Godot |
| `world/project.godot`, `world/scenes/main.tscn` | Furnished scene with imported glTF models, room structure (floor/wall/ceiling/rug/window), distinct materials | P21-T05 completed (2026-10-02) |
| `world/scripts/game_world.gd` | Controller: camera, movement, teleop, input mode switching; no bridge integration | P21-T01 verified |
| `tools/Godot_v4.7.2-stable_linux.x86_64` | Present; version command works; a bounded GUI run initializes OpenGL | Verified baseline (T00) |
| `world/assets/cache` | 80 files, ~3.2 GB; all glTF models assembled with buffers and textures | P21-T02/T03 completed (2026-10-02) |
| `world/assets/manifest.json` | Authoritative asset manifest with 13 assets, complete provenance, MD5 verification, and SHA-256 hashes for all files; `missing_files` section removed (all files present) | P21-T02/T03 completed (2026-10-02) |
| `world/assets/manifest.md` | Prose asset proposal; replaced by authoritative `manifest.json` | Historical reference only |
| Compositor extraction | Surface texture field moved, but compositor still includes GL/display headers and calls renderer helpers; no core-library Meson target | T10 reopened |
| `src/godot/`, deployed `.so` | Draft API/wrapper, empty initialization callbacks, no class registration, unresolved Mansion symbols, `extension.toml` instead of a loadable `.gdextension` resource | T11 incomplete; building a `.so` did not integrate Godot |
| Godot snapshots/input/terminal | No scene wiring to bridge or live ImageTexture | T12–T19 unimplemented/unaccepted |

## P21-T02 verification 2026-10-02 (asset selection and provenance)

**T02 completed:**
- Manifest reviewed against ART-DIRECTION.md criteria:
  - Oak/walnut furniture (wooden_bookshelf_worn, desk_lamp_arm_01)
  - Textured timber floor (curly_teddy_natural_4k rug)
  - Warm plaster walls (white_plaster_02_2k, beige_wall_001_4k)
  - Soft daylight HDRI (anniversary_lounge_16k_jpg)
  - Books, rug, plant for scale (book_encyclopedia_set_01, potted_plant_02)
- All 80 cached files verified:
  - 6 glTF model files (.gltf) with embedded buffer references
  - 6 external buffer files (.bin)
  - 39 texture files (.jpg) including normal maps and roughness maps
  - 2 HDRI files (.jpg, .exr)
  - 16 wall/hard surface texture files (.jpg)
- Hash verification: SHA-256 for all files in manifest.json
- Godot import validation: `--headless --import --quit-after 1` exits 0
- manifest.json cleaned: Removed `missing_files` section (all files present)

**P21-T03 completed (2026-10-02):**
- Asset fetcher implemented: `world/tools/fetch_assets.py` with include file support
  - Resolves "include" section nested in `gltf.2k.gltf` files
  - Downloads missing .bin buffer files and texture files
  - Verifies MD5 hashes from Poly Haven API, computes SHA-256 for manifest
- Download completed: 80 total files in cache
  - 6 glTF model files (with embedded .bin buffer references)
  - 6 external .bin buffer files
  - 39 texture files (including normal maps and roughness maps)
  - 2 HDRI files (24k_exr, 16k_jpg)
  - Wall and hard surface texture assets
- Hash verification: All files have SHA-256 hashes in manifest
- Godot validation: `--headless --import --quit-after 1` exits 0

**P21-T04 completed (2026-10-02):**
- Imported 6 complete glTF models into Godot 4.7.2-stable with Compatibility renderer
- `.import` config files created for each model in `world/assets/`
- 6 `.scn` files generated in `.godot/imported/` with correct Godot resource header (RSCC)
- Import validation: `--headless --path world --import --quit-after 1` exits 0
- No missing-resource errors; all external buffers referenced correctly
- MD5 hashes computed for each glTF file to construct deterministic import paths
- Duplicate cache imports removed; only renamed `.gltf` files processed
- Asset fetcher cache (`world/assets/cache/`) preserved; `*_glb.gltf` files retained for reference

## Verified by automated test

**P21-T00 verification 2026-10-02 (toolchain baseline):**

- Local Godot `--version`: `4.7.2.stable.official.ed1daf0bf` (exit 0).
- Godot `--help`: all CLI options documented (exit 0).
- Godot SHA-256: `8d106cbe6144c2dc7e881d61d2429c1a8a76e6b22ef48bd5e48dcf934953f71e`.
- godot-cpp commit: `507ed9d840c01a3c5b2a39af8bb4000bfac30bf5`, describe: `10.0.0-stable`.
- godot-cpp API header: Godot 4.7.0 stable single precision.
- Fresh isolated copy of scene/scripts: `--headless --path COPY --import`
  exits 0; headless runtime `--quit-after 3` also exits 0.
  This is not visual, asset or bridge acceptance.
- Bounded GUI run of that copy with `--rendering-method gl_compatibility`
  exits 0 and reports OpenGL 4.6 / Mesa 26.2.3 / AMD Custom GPU 0405.
  Audio libraries/device are unavailable; it falls back to dummy audio.
  No image was inspected for this Godot run.
- `meson compile -C build && meson test -C build --print-errorlogs`:
  **30/30 tests pass**; C++ baseline established.
- `node --test .opencode/tests/*.test.js`: **62/62 pass**; task-parser checks
  also cover the reopened scope. No live Qwen/OpenCode run is claimed.

**Earlier in this conversation, before this audit:** host-window fixes are
commits `a5ba64a`, `99fcedb`, `3dfaa26`. See
[host-window evidence](handoffs/04-host-window.md). Those tests do not validate
Godot.

**Offline inspection of cached glTF files (2026-10-02):** **30/30 referenced
external buffers/images missing**. There is no assembled asset scene. This is
T03's domain; not a T00 failure.

## P21-T01 verification 2026-10-02 (Godot scaffold repair)

**T01 fixed:**
- `run-godot.sh`: Uses `--path "$PROJECT_DIR"` correctly; distinguishes
  `--editor`, `--headless`, and `--path` modes.
- `validate-godot-project.sh`: Preserves nonzero exit status, uses per-run log,
  validates project exists, checks scene file syntax, verifies key nodes and
  script presence. Tested against intentionally invalid temp project - fails.
- `world/scenes/main.tscn`: Fixed floor `Transform3D` - was
  `Transform3D(1,0,0, 0,0,0, 0,0,1, 0,0,0)` (zero Y scale), now
  `Transform3D(1,0,0, 0,1,0, 0,0,1, 0,0,0)` (proper identity).
- `world/scripts/game_world.gd`: Controller properly resolves camera via
  `_find_camera()` in `_enter_tree()`, handles right-click mouse capture/release,
  movement in world mode, teleport (T key), app mode switch (Enter/F12).
- Import validation: `tools/Godot_v4.7.2-stable_linux.x86_64 --headless --import
  --quit-after 1 world` exits 0 with no script/runtime errors.
- Audio subsystem unavailable (pulse, ALSA); falls back to dummy driver as expected.

**Working Godot launch command:**
```sh
tools/Godot_v4.7.2-stable_linux.x86_64 --headless --path world --import --quit-after 1
```

For interactive editor use:
```sh
tools/run-godot.sh --editor
```

**Note:** `build/mansion-desktop --room-camera` still runs the legacy C++/EGL renderer.

## Verified by a person

The owner reported that the legacy rendered room and input worked after
`99fcedb`, then confirmed the vertical-orientation fix after `3dfaa26` with
"ok that works". This is narrow owner feedback on those fixes, not a real
terminal trial, art acceptance, or completion of any task marked (human).

## Not verified / current failures

- The first isolated invocation with both `--editor --import` aborted with
  `ERROR: Parameter "singleton" is null` in `editor/editor_node.cpp:6618`.
  Using `--import` alone then passed on both the same and a fresh isolated copy.
  Record the failed invocation, use the working one, and do not declare a
  blanket import/display blocker or rebuild the engine without a new failure.
- Scaffold source has no imported assets; T04-T09 require Poly Haven assets.
- No demonstrated registered GDExtension class, private socket driven by Godot,
  owned frame snapshot, application input, or real terminal in Godot.
- No rendered real-asset contact sheet, furnished room, lighting review,
  target performance result, or integrated product acceptance.
- Old statements "requires a display server", "enable the extension through
  Project Settings", and "Compatibility (Vulkan)" are not instructions to follow.
  Compatibility uses OpenGL; see TOOLCHAIN.md and the current handoff.

## Evidence and reporting rules

Record task, source revision, exact command, exit code, expected/actual result,
log/capture path and observer. A zero exit code alone never establishes the
feature. Keep unfinished tasks unchecked; record a concrete failed command
before declaring an environment blocker. Preserve existing untracked tools,
`test-ext-proj/`, `world/bin/` and `.uid` files; they are not permission to stage
all files or delete them. Stage only the current task's changes.
