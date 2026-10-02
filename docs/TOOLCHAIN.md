# Toolchain inventory and verification work

Updated 2026-10-02 against `3dfaa26`. **P21-T00 is reopened.** This is an
observed local inventory, not a completed reproducible upstream bootstrap.
The old "latest stable", release dates, archive hashes/download recipes and
Blender license claims were not substantiated and must not be reused as pins.
Do not delete/re-download working local tools merely to match an old document.

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

## Observed local tools

| Item | Observation on 2026-10-02 | What remains to verify |
| --- | --- | --- |
| `tools/Godot_v4.7.2-stable_linux.x86_64` | Executable, 146414384 bytes; `--version` returns `4.7.2.stable.official.ed1daf0bf` | Upstream provenance/checksum, full-project validation and repeatable bootstrap |
| Binary SHA-256 | `8d106cbe6144c2dc7e881d61d2429c1a8a76e6b22ef48bd5e48dcf934953f71e` measured locally | This is an identity fingerprint, not a verified publisher checksum |
| `tools/godot-4.7.2-stable/` | Source tree exists; `version.py` says 4.7.2 stable | Relation to the binary, origin and reproducible build |
| `tools/godot-cpp/` | Local Git commit `507ed9d840c01a3c5b2a39af8bb4000bfac30bf5`; describe says `10.0.0-stable` | Provenance, compatible build configuration and actual extension load |
| `tools/godot-cpp/gdextension/extension_api-4-7.json` | Header identifies Godot 4.7, patch 0, single precision | Choose the correct API/build target and verify with the actual runtime |
| Blender | `command -v blender` found nothing; no local executable identified in audit | Optional: needed only if chosen source formats require conversion |

A godot-cpp release tag need not share the engine's version number. Check the
API it targets and prove loadability; do not infer incompatibility from the tag
alone. Do not link against a Godot executable. Build the reusable Mansion core
as a library during T10, and the GDExtension against that library during T11.

## Commands that were actually probed

From repository root:

```sh
tools/Godot_v4.7.2-stable_linux.x86_64 --version
tools/Godot_v4.7.2-stable_linux.x86_64 --help
sha256sum tools/Godot_v4.7.2-stable_linux.x86_64
git -C tools/godot-cpp rev-parse HEAD
git -C tools/godot-cpp describe --always --tags
```

The local `--help` documents `--path DIRECTORY`, `--import` and `--quit-after N`.
`tools/run-godot.sh` currently passes `--project`; repair it in T01 rather than
copying it as a known-good invocation. The validator also needs repair; it hides
exit failures with `|| true` and passes the project directory incorrectly.

For isolated diagnostics, copy only `world/project.godot`, `world/scenes/` and
`world/scripts/` to a temporary directory under `/tmp`. Do not import the 2.8 GB
cache while checking the editor itself. Substitute that directory for
`/tmp/PROJECT_COPY` in these diagnostic commands (not accepted product checks):

```sh
timeout -k 2s 30s tools/Godot_v4.7.2-stable_linux.x86_64 \
  --headless --path /tmp/PROJECT_COPY --import
timeout -k 2s 15s tools/Godot_v4.7.2-stable_linux.x86_64 \
  --headless --path /tmp/PROJECT_COPY --quit-after 3
timeout -k 2s 15s tools/Godot_v4.7.2-stable_linux.x86_64 \
  --path /tmp/PROJECT_COPY --rendering-method gl_compatibility --quit-after 3
```

Audit results: `--import` alone exits 0 on a fresh isolated project copy;
headless runtime exits 0; GUI runtime initializes OpenGL and exits 0 with missing
audio-library/device warnings and dummy fallback. The first invocation using
both `--editor --import` aborted with a null `singleton` in
`editor/editor_node.cpp:6618`; the successful commands above supersede a blanket
import blocker. Do not infer its root cause solely from these runs. No rendered
Godot image was inspected. Use the documented
`--audio-driver Dummy` for subsequent visual smoke checks if needed; do not
install host audio packages to solve this prototype's missing sound.

## Required T00 deliverable

Replace this provisional inventory with a tested configuration: source URLs,
verified upstream tags/commits, downloaded-file hashes and licenses; local
bootstrap/build commands; version/API/renderer checks; clean editor import and
runtime proof. Preserve measured local fingerprints separately from upstream
verification. Reuse existing bytes only after verification. If a replacement is
needed, obtain it from official upstream metadata, verify it and keep it local.
Do not invent checksums, copy a checksum from a different version, install system
packages, or rebuild the entire engine before diagnosing the current binary.

Blender is optional for complete glTF. Pin and test its real release, provenance,
license and exporter only when a selected source requires it. No Blender GUI is
required for ordinary glTF import or scripted batch conversion.

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
