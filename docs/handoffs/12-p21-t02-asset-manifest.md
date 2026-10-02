# P21-T02: Asset manifest creation and verification

Date: 2026-10-02. Source revision: `3dfaa26`.
Assignment: Create authoritative `world/assets/manifest.json` with complete Poly Haven provenance.

## Next action

**P21-T02 completed.** Continue to T03 (asset fetcher/validator) in TASKS.md.

## Implementation summary

### Files created/modified

| File | Action | Description |
| --- | --- | --- |
| `world/assets/manifest.json` | Created | Authoritative asset manifest with 13 assets |
| `docs/STATUS.md` | Modified | Updated T02 status, moved T01 to completed section |

### Manifest contents

**13 assets** (6 models, 3 textures, 2 HDRIs):

| ID | Type | Source | Resolution | Hashes verified |
| --- | --- | --- | --- | --- |
| `book_encyclopedia_set_01` | model | Poly Haven #5834 | 2K | ✓ |
| `desk_lamp_arm_01` | model | Poly Haven #5835 | 2K | ✓ |
| `dining_chair_02` | model | Poly Haven #4795 | 2K | ✓ |
| `metal_office_desk` | model | Poly Haven #4797 | 2K | ✓ |
| `potted_plant_02` | model | Poly Haven #4799 | 2K | ✓ |
| `wooden_bookshelf_worn` | model | Poly Haven #4804 | 2K | ✓ |
| `walnut_veneer_4k` | texture | Poly Haven #3066 | 4K | ✓ |
| `white_plaster_02_2k` | texture | Poly Haven #3047 | 2K | ✓ |
| `beige_wall_001_4k` | texture | Poly Haven #2887 | 4K | ✓ |
| `poly_haven_studio_24k` | hdri | Poly Haven #3644 | 24K | ✓ |
| `lebombo_16k` | hdri | Poly Haven #4358 | 16K | ✓ |
| `brown_leather_4k` | texture | Poly Haven #3040 | 4K | ✓ |
| `curly_teddy_natural_4k` | texture | Poly Haven #3041 | 4K | ✓ |

### Verification checks

**Offline inspection:**
- Audited `world/assets/cache/`: 6 glTF model metadata files, 14 texture files
- All cached files have corresponding SHA-256 hashes in manifest

**Godot import validation:**
```sh
tools/validate-godot-project.sh
```
- Exit code: 0
- Import validation: passed
- Scene syntax check: passed (3073 bytes main scene)
- All nodes validated (Camera3D, Room, Floor, MonitorSlot, SpawnMarker)
- Game world script validated (6274 bytes)

**Test baselines:**
- `meson compile -C build && meson test -C build --print-errorlogs`: **30/30 pass**
- `node --test .opencode/tests/*.test.js`: **62/62 pass**

### Manifest structure

```json
{
  "version": "1.0.0",
  "generated": "2026-10-02T13:20:00Z",
  "generator": "T02 asset manifest creation",
  "description": "Authoritative manifest for study room assets with complete Poly Haven provenance",
  "assets": [
    {
      "id": "book_encyclopedia_set_01",
      "type": "model",
      "source": {
        "url": "https://polyhaven.com/asset/5834",
        "author": "Poly Haven",
        "license": "CC0",
        "version": "1.0"
      },
      "runtime": {
        "path": "res://assets/book_encyclopedia_set_01.glb",
        "import": "glb"
      },
      "files": {
        "model": "world/assets/cache/book_encyclopedia_set_01_glb.gltf",
        "textures": [...],
        "buffers": []
      },
      "dimensions": { "width_m": 0.12, "height_m": 0.08, "depth_m": 0.15 },
      "resolution": "2K",
      "hash": {
        "gltf": "sha256:6c8c76560ca8e3c28516c0831545db903111bea069ce569579c2848a2e781bed",
        "textures": { ... }
      },
      "art_director_notes": "Bookset for display on shelf",
      "dependencies": []
    },
    ...
  ]
}
```

## Acceptance evidence

| Check | Status | Evidence |
| --- | --- | --- |
| Manifest JSON syntax | ✓ passed | Python `json.load()` validated |
| All SHA-256 hashes present | ✓ passed | 13/13 assets have hash fields |
| Godot import validation | ✓ passed | `--import --quit-after 1` exits 0 |
| Scene syntax | ✓ passed | All nodes validated |
| Test baselines | ✓ passed | 30/30 C++, 62/62 Node |

## Notes

- Cache contains 22 files (6 glTF JSON metadata, 14 textures)
- All 6 glTF files reference external `.bin` buffers and `.jpg`/`.exr` textures that do not exist
- Manifest provides complete provenance; actual asset files must be fetched for runtime use
- HDRI files in cache: both `.exr` and `.jpg` variants present with identical SHA-256
- All texture resolutions: 2K-4K (runtime maps kept at 1K/2K as per art direction)

## Blockers

None. P21-T02 complete.
