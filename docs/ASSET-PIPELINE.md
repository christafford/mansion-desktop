# Reproducible Poly Haven asset pipeline

Complete the downloader, manifest and conversion tools during Project 21.
The 2026-10-02 audit found 30 missing external references. The six model packages
now exist locally and render in Godot. `world/tools/prepare_study_assets.py`
checks their manifest SHA-256 values and stages complete source glTF packages.
The source cache has `.gdignore`; runtime copies are generated and Git-ignored.
This local staging does not complete acquisition/provenance/offline negative-path
acceptance in T02/T03. Preserve source downloads; GLB conversion is not required.

## Select and record

Start with a curated study set: timber flooring, plaster, furniture materials,
desk/table, chair, lamp, shelves/books, small props and one suitable HDRI.
Inspect the live [catalog](https://polyhaven.com/all) and candidate previews.
Use compatible free assets from another documented provider for catalog gaps.
Create and keep `world/assets/manifest.json` authoritative (present; provenance/runtime metadata still needs review).
Every used asset records:

- stable provider/asset ID, source page, author, license and license URL;
- exact downloadable files, resolution/format, URLs, bytes and SHA-256 checksums;
- local source/cache paths and final runtime paths;
- conversion commands, pinned tool version, coordinate/unit changes and LOD;
- texture channels, color space, normal convention and final material mapping.

Checksums lock a reviewed download; do not invent them or silently replace a
changed remote file. Keep original source material reproducible. Large raw
sources and caches stay ignored; decide explicitly whether compact runtime
assets are tracked or fetched into an ignored directory. Document a fresh-clone
bootstrap and offline behavior. Missing assets produce actionable errors,
not a silent accepted return to primitive furniture.

## Access the service correctly

Use the official [API documentation](https://api.polyhaven.com/) and
[API terms](https://github.com/Poly-Haven/Public-API/blob/master/ToS.md) at
implementation time. Asset license and hosted-service terms differ:
[downloaded assets are CC0](https://polyhaven.com/license). Live API access
requires an identifying User-Agent/Referer and source credit when surfacing its
content. Make provenance visible in the asset tooling; if an in-app browser is
ever added, include its visible Poly Haven credit. Offline runtime need not call
the API. Recheck actual requirements; don't apply obsolete commercial-access
claims from old articles.

Identify this project's requests, cache metadata, download only the curated set,
limit concurrency, back off on throttling and retry transient errors with a
finite retry policy. Read real metadata for file URLs and included texture
dependencies; don't scrape an HTML download button or guess CDN paths. Write
downloads atomically, validate hashes/size before import, and reject unsafe
archive paths. Interrupted runs resume verified files instead of downloading
the catalog again. Implement `--dry-run`, selective fetch, checksum verification
and deterministic validation so unattended agents can diagnose failure.

## Convert and import

Use glTF/GLB for runtime models where practical. If sources are `.blend`, use a
pinned Blender batch exporter into GLB so launch/import doesn't depend on a
developer's ambient Blender install. Test an export before converting all assets.
Keep meaningful nodes, UVs, textures, pivots, scale and material assignments.
Record Godot import settings instead of hand-editing generated `.godot` caches.

Use meters and Godot's actual coordinate conventions, and verify by rendering a
known scale/orientation fixture. Create simple deliberate collision geometry;
do not make scanned furniture's full triangle mesh the default moving collider.
Make furniture scenes reusable. Model monitor housing and its screen slot as
distinct children of the desk/monitor transform. The application panel's UVs,
aspect and collision/picking surface are explicit and testable.

Map albedo to sRGB and data maps (normal/roughness/metalness/AO) to their correct
linear channels. Verify channel packing and normal-map handedness from the
actual source and Godot import behavior. Do not blindly connect a displacement
texture to a normal input. Preserve roughness variation and sensible physical
texture scale. Start with 1K/2K runtime textures and modest HDRI resolution;
upgrade focal assets only with measured benefit. Source downloads can be larger
but should not all become runtime textures. Use LODs/instancing/compression
after checking import quality, silhouettes and memory on the target renderer.

## Verify

Check manifest completeness, missing references, hashes, license/provenance,
conversion reproducibility and offline import. Render a contact sheet in the
engine showing every runtime model/material before room composition. Review
that sheet for broken materials, lost textures, unit errors and flipped normals.
No task passes merely because files downloaded. Record the selected assets,
tools, commands and actual rendered evidence in `docs/ACCEPTANCE.md`.
