# Project status

Current authority: 2026-10-02 source/runtime audit against `3dfaa26`.
This file replaces the contradictory older status; that material is retained
only as [historical evidence](handoffs/10-status-before-godot-audit.md).
Use the checklists in [TASKS.md](TASKS.md), not old handoff completion claims.
Decision [06](decisions/06-godot-poly-haven.md) remains the product direction.

## Next task

**P21-T03 — fetch and validate all Poly Haven assets for study room.**
T03 downloads missing asset files (glTF .bin buffers and textures) from Poly Haven
API, verifies MD5 hashes from the API response, and computes SHA-256 for the
manifest. Godot validation ensures all assets import successfully.

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
| `world/project.godot`, `world/scenes/main.tscn` | Separate scaffold: BoxMesh furniture, solid materials, no imported asset references | T01 reopened; furnished room absent |
| `world/scripts/game_world.gd` | Navigation draft; mode flag does not forward application input | No live terminal integration |
| `tools/Godot_v4.7.2-stable_linux.x86_64` | Present; version command works; a bounded GUI run initializes OpenGL | Verified baseline (T00) |
| `world/assets/cache` | 80 files, ~3.2 GB; all glTF models assembled with buffers and textures | P21-T03 completed (2026-10-02) |
| `world/assets/manifest.json` | Authoritative asset manifest with 13 assets, complete provenance, MD5 verification, and SHA-256 hashes for all files | P21-T03 completed (2026-10-02) |
| `world/assets/manifest.md` | Prose asset proposal; replaced by authoritative `manifest.json` | Historical reference only |
| Compositor extraction | Surface texture field moved, but compositor still includes GL/display headers and calls renderer helpers; no core-library Meson target | T10 reopened |
| `src/godot/`, deployed `.so` | Draft API/wrapper, empty initialization callbacks, no class registration, unresolved Mansion symbols, `extension.toml` instead of a loadable `.gdextension` resource | T11 incomplete; building a `.so` did not integrate Godot |
| Godot snapshots/input/terminal | No scene wiring to bridge or live ImageTexture | T12–T19 unimplemented/unaccepted |

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
- `tools/run-godot.sh` uses `--project`; the local engine documents `--path`.
  `tools/validate-godot-project.sh` uses the wrong project argument placement,
  suppresses command failure with `|| true` and only greps logs. Repair in T01.
- Scaffold source has suspect material assignments, duplicate autoload/class
  naming and mouse event handling. These are T01 audit leads, not independently
  reproduced runtime failures. No captured scaffold-quality pass exists.
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
