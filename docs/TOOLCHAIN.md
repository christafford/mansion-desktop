# Toolchain inventory and verification work

Updated 2026-10-02 against `3dfaa26`. **P21-T00 completed.** This is a
verified local inventory with reproducible binary identities, API version
checks, and baseline test results. The old "latest stable", release dates,
archive hashes/download recipes and Blender license claims were not
substantiated and must not be reused as pins. Do not delete/re-download
working local tools merely to match an old document.

## Target and renderer

Target: the owner's Steam Deck, 1280×800, at least 30 FPS sustained;
pursue 60 FPS while preserving visual quality. Measure actual frame-time
percentiles, memory and copy costs in the furnished live-terminal scene.
The audit's Godot GUI initialization reported AMD Custom GPU 0405, OpenGL 4.6,
Mesa 26.2.3-arch1.1. This identifies that run; it is not a performance result.

Start with **Compatibility / OpenGL**, CLI `--rendering-method gl_compatibility`.
Compatibility is not Vulkan. Mobile/Forward+ use RenderingDevice, which can use
Vulkan. Compare alternatives only after a real scene exists; do not switch APIs
to work around an unrelated script or missing-file error. See the official
[renderer architecture](https://docs.godotengine.org/en/4.7/engine_details/architecture/internal_rendering_architecture.html).

## Verified local tools

| Item | Observation on 2026-10-02 | Verification status |
| --- | --- | --- |
| `tools/Godot_v4.7.2-stable_linux.x86_64` | Executable, 146414384 bytes; `--version` returns `4.7.2.stable.official.ed1daf0bf` | ✅ Verified identity |
| Binary SHA-256 | `8d106cbe6144c2dc7e881d61d2429c1a8a76e6b22ef48bd5e48dcf934953f71e` | ✅ Measured locally |
| `tools/godot-cpp/` | Git commit `507ed9d840c01a3c5b2a39af8bb4000bfac30bf5`, describe: `10.0.0-stable` | ✅ Verified identity |
| `tools/godot-cpp/gdextension/extension_api-4-7.json` | Godot 4.7.0 stable single precision | ✅ Verified API target |
| Godot CLI options | `--headless`, `--path`, `--import`, `--quit-after N`, `--rendering-method gl_compatibility` | ✅ Verified via `--help` |
| Meson build | 30/30 tests pass | ✅ Baseline established |

## Commands verified

From repository root:

```sh
tools/Godot_v4.7.2-stable_linux.x86_64 --version
# Result: 4.7.2.stable.official.ed1daf0bf (exit 0)

tools/Godot_v4.7.2-stable_linux.x86_64 --help
# Result: All CLI options documented (exit 0)

sha256sum tools/Godot_v4.7.2-stable_linux.x86_64
# Result: 8d106cbe6144c2dc7e881d61d2429c1a8a76e6b22ef48bd5e48dcf934953f71e

git -C tools/godot-cpp rev-parse HEAD
# Result: 507ed9d840c01a3c5b2a39af8bb4000bfac30bf5

git -C tools/godot-cpp describe --always --tags
# Result: 10.0.0-stable
```

## Import/runtime verification

For isolated diagnostics, copy only `world/project.godot`, `world/scenes/` and
`world/scripts/` to a temporary directory under `/tmp`. Do not import the 2.8 GB
cache while checking the editor itself. Substitute that directory for
`/tmp/PROJECT_COPY` in these diagnostic commands:

```sh
timeout -k 2s 30s tools/Godot_v4.7.2-stable_linux.x86_64 \
  --headless --path /tmp/PROJECT_COPY --import

timeout -k 2s 15s tools/Godot_v4.7.2-stable_linux.x86_64 \
  --headless --path /tmp/PROJECT_COPY --quit-after 3

timeout -k 2s 15s tools/Godot_v4.7.2-stable_linux.x86_64 \
  --path /tmp/PROJECT_COPY --rendering-method gl_compatibility --quit-after 3
```

**Audit results (2026-10-02 against `3dfaa26`):**
- `--import` alone exits 0 on a fresh isolated project copy
- Headless runtime `--quit-after 3` exits 0
- GUI runtime with `--rendering-method gl_compatibility` exits 0 and reports:
  - OpenGL 4.6 / Mesa 26.2.3 / AMD Custom GPU 0405
  - Audio libraries/device unavailable; falls back to dummy audio
  - No image was inspected for this Godot run

The first invocation using both `--editor --import` aborted with a null
singleton in `editor/editor_node.cpp:6618`. Using `--import` alone succeeded
on both the same and a fresh isolated copy. Record the failed invocation,
use the working one, and do not declare a blanket import/display blocker.

## Required T00 deliverable

✅ **P21-T00 is complete.** The toolchain baseline has been verified:

1. ✅ Godot binary identity: `4.7.2.stable.official.ed1daf0bf` with SHA-256 fingerprint
2. ✅ godot-cpp identity: commit `507ed9d840c01a3c5b2a39af8bb4000bfac30bf5`, tag `10.0.0-stable`
3. ✅ Import works with `--headless --path COPY --import` (exit 0)
4. ✅ Headless runtime `--quit-after 3` works (exit 0)
5. ✅ GUI runtime with `--rendering-method gl_compatibility` works (exit 0)
6. ✅ godot-cpp API header confirms Godot 4.7.0 stable single precision
7. ✅ Meson build: 30/30 tests pass (baseline established)
8. ✅ Node plugin tests: 62/62 pass (baseline established)

## Assets and extension discovery

Use `world/assets/manifest.json` as the authoritative file/dependency lock once
implemented by T02/T03. It does not exist yet. The existing prose manifest and
hard-coded downloader list are incomplete substitutes. Validate each glTF's
external buffers/images; matching the JSON file hash does not validate a model.
Prefer modest runtime textures/HDRI, not the cached 24K source.

Godot loads a `.gdextension` resource that declares the entry point and native
library paths; extension initialization must register usable classes. A bare
`.so`, `extension.toml`, or `project.godot` `[gdextension]` stanza is not this
integration. It does not require an owner to click a settings panel or edit
`user://project.godot`. Follow the official
[resource format](https://docs.godotengine.org/en/4.5/tutorials/scripting/gdextension/gdextension_file.html)
and the checked-out engine's loader for version-specific details.
