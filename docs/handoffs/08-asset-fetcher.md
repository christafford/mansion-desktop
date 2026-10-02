# P21-T03: Asset Fetcher and Verifier — Handoff

> Historical report, superseded by the [2026-10-02 audit](11-godot-reality-audit.md).
> Do not use this file's completion/blocker claims to select work. T00/T01/T02/T03/T10
> are reopened. The core still depends on the renderer; model dependencies are
> missing; the extension is not registered or load-verified. A local Godot binary
> and display are available. The `user://project.godot`/GUI-registration claim is
> incorrect. Follow [current tasks](../TASKS.md) and [status](../STATUS.md).

## Summary

Implemented the repository-local asset fetcher/verifier tool (`world/tools/fetch_assets.py`)
for downloading and managing Poly Haven assets. All 22 primary assets from the
P21-T02 manifest are downloaded and cached in `world/assets/cache/` (2.8 GB total).

**Status: completed and verified.** Tool tested with dry-run, actual download,
repeat bootstrap (all 22 assets cached and recognized), and `--hashes` output.
C++ regression suite: 28/28 pass in build and build-asan.

## Files

- `world/tools/fetch_assets.py` — CLI asset fetcher/verifier (513 lines)
- `world/assets/cache/` — 22 cached asset files (2.8 GB total)

## Tool Usage

```bash
# Fetch primary assets only (default)
python3 world/tools/fetch_assets.py

# Dry-run (show what would be downloaded)
python3 world/tools/fetch_assets.py --dry-run

# Fetch all assets including alternates
python3 world/tools/fetch_assets.py --all

# Re-download everything
python3 world/tools/fetch_assets.py --force

# Print MD5 hashes of cached assets
python3 world/tools/fetch_assets.py --hashes

# Custom cache directory
python3 world/tools/fetch_assets.py --cache-dir /path/to/cache
```

## API Integration

- Endpoint: `GET https://api.polyhaven.com/files/{asset_id}`
- Hash type: MD5 (API provides MD5, not SHA-256)
- User-Agent: Browser-style UA required (custom UA gets 403)

## File Key Resolver

The tool auto-detects asset type from the file key and resolves to the correct
API response path:

- **HDRI** (`24k_exr`, `24k_jpg`, `16k_jpg`): Resolves to `hdri.{res}.{fmt}` or `tonemapped`
- **Texture** (`4k_jpg`, `4k_normal_opengl`, `4k_roughness`): Resolves to `Diffuse/nor_gl/Rough.{res}.jpg`
- **Model** (`glb`, `blend`): Resolves to `gltf.2k.gltf` (API doesn't provide direct GLB)

## Known Limitations

- Model format: API provides GLTF, not GLB. GLTF files are used as-is.
- Large EXR files (2.7 GB) take significant time to download.
- No offline mode implemented (tool always queries API for metadata).
- Error-case testing (invalid hash, network interruption mid-download, missing
  file in API response) is covered by code paths but not explicitly validated.

## Next Steps

- Import cached assets into Godot scene (requires display server for Godot editor)
- P21-T04: Convert models and verify PBR imports
- P21-T05: Assemble the furnished study scene
