# Elsewhere — Study Asset Manifest

> 2026-10-02 audit: this is a **candidate selection**, not a verified runtime
> manifest. T02/T03 are reopened. All six cached glTF files lack their external
> buffers/images. Review previews/provenance, complete file dependencies and
> create `world/assets/manifest.json` before claiming these assets are imported.
> Follow [the current work order](../../docs/TASKS.md).

> **P21-T02** — Curated Poly Haven asset set for the warm study room.
> Generated 2026-10-01 from live Poly Haven API data.

## 1. Scene Composition Layout

```
┌──────────────────────────────────────────────────────────────────────┐
│                                                                      │
│  ┌─────────┐              ┌───────────────────────────────┐          │
│  │         │              │                               │          │
│  │  BOOK-  │              │          DESK                  │          │
│  │  CASE   │              │    ┌───────────┐  ┌─────────┐ │          │
│  │         │              │    │  MONITOR  │  │  DESK   │ │          │
│  │         │              │    │  SLOT     │  │  LAMP   │ │          │
│  │         │              │    └───────────┘  └─────────┘ │          │
│  │         │              │                               │          │
│  └─────────┘              │         ┌─────────┐           │          │
│   [Z: +3m]                │         │  CHAIR  │           │          │
│                           │         └─────────┘           │          │
│                           │                               │          │
│                           │                               │          │
│                           │         ┌──┐  ┌───┐           │          │
│                           │         │  │  │RUG│           │          │
│                           │         └──┘  └───┘           │          │
│                           │                               │          │
│                           └───────────────────────────────┘          │
│                                                                      │
│  Camera: SpawnMarker near door corner, looking into room             │
│  Window: Right wall (positive X) — natural daylight                  │
│  Lamp: Warm accent on desk — artificial fill                         │
│                                                                      │
└──────────────────────────────────────────────────────────────────────┘
```

**Room dimensions:** ~4.5m × 5m × 2.8m (W × D × H) — proportional to a real study.

---

## 2. HDRI (Lighting / Environment)

| Priority | Asset ID | Type | Resolution | EVs Cap | Description | Author | License |
|----------|----------|------|------------|---------|-------------|--------|---------|
| **Primary** | `poly_haven_studio` | HDRI | 24576 × 12288 | 12 | Bright home office, soft natural daylight mixed with down lights, low contrast. Tags: chair, office, table, heater, cat, wooden ceiling, computer, window, lamp, curtain, tiles, desk, room, bright. | Greg Zaal | CC0 |
| **Alt 1** | `anniversary_lounge` | HDRI | 21504 × 10752 | 10 | Warm window daylight mixed with yellow ceiling lamps, medium-contrast cozy lounge. Tags: window, room, couch, lamp, carpet, table. | — | CC0 |
| **Alt 2** | `lebombo` | HDRI | 16384 × 8192 | 9 | Empty sunlit living room, soft morning light through sheer curtains, warm wall lamps, low contrast. Tags: house, lamp, window, wood floor, empty. | — | CC0 |
| **Alt 3** | `living_room_01` | HDRI | — | — | Warm natural light with lamp and window. *(Not in catalog; listed for reference)* | — | — |

**Selection rationale:** `poly_haven_studio` is the best match — it is literally a home office with a desk, chair, lamp, and window. The low-contrast lighting is forgiving for real-time rendering on Steam Deck. Resolution up to 24K provides excellent lighting fidelity.

---

## 3. PBR Textures

### 3.1 Floor — Warm Oak / Walnut Planks

| Priority | Asset ID | Resolution | Description | Tags | Author | License |
|----------|----------|------------|-------------|------|--------|---------|
| **Primary** | `walnut_veneer` | 16384 × 16384 | Walnut veneer: smooth, fine grain, raw wood surface, warm timber | smooth, grain, wood grain, raw, raw wood, veneer, veneer wood, walnut | Poly Haven | CC0 |
| **Alt 1** | `wooden_floor_01` | 16384 × 16384 | Lacquered, varnished wooden floor, natural grain, subtle weathering | weathered, wood, wooden plank, lacquered, coated finish, coated wood, coated, indoor | Poly Haven | CC0 |
| **Alt 2** | `plank_flooring_04` | 8192 × 8192 | Varnished dark timber planks, subtle grain, narrow seams, soft satin sheen | wooden planks, plank flooring, wooden flooring, plank, timber, coated, varnished, wooden floor | Poly Haven | CC0 |

### 3.2 Walls — Walnut Paneling

Owner direction, 2026-10-06: the visible study now reuses `walnut_veneer`
from the verified local photo cache on 65cm panel bays with recessed joints and
rails. No new download or license. The plaster candidates below are historical.
See [T57 evidence](../../docs/handoffs/31-furnishings-and-transitions.md).

| Priority | Asset ID | Resolution | Description | Tags | Author | License |
|----------|----------|------------|-------------|------|--------|---------|
| **Primary** | `beige_wall_001` | 16384 × 16384 | Smooth beige painted plaster, subtle grain, matte finish, soft color variation | beige, painted, smooth, suburb, house, home | Poly Haven | CC0 |
| **Alt 1** | `white_plaster_02` | 8192 × 8192 | Smooth, flat white plaster, soft matte finish, fine micro-roughness, subtle dirt specks | smooth, flat, even | Poly Haven | CC0 |
| **Alt 2** | `yellow_plaster` | 8192 × 8192 | Warm ochre plaster, smooth worn surface, subtle scratches, ochre to brown tones | yellow, plastered, worn, scratched, smooth, concrete, cement, moss | Poly Haven | CC0 |

### 3.3 Ceiling — Light Plaster / Painted

| Priority | Asset ID | Resolution | Description | Tags | Author | License |
|----------|----------|------------|-------------|------|--------|---------|
| **Primary** | `white_plaster_02` | 8192 × 8192 | Smooth flat white plaster — matches wall alt, ceiling can be lighter | smooth, flat, even | Poly Haven | CC0 |
| **Alt** | `beige_wall_001` | 16384 × 16384 | Same as wall (ceiling can share wall texture with adjusted color) | — | Poly Haven | CC0 |

### 3.4 Rug / Carpet — Warm Fleece

| Priority | Asset ID | Resolution | Description | Tags | Author | License |
|----------|----------|------------|-------------|------|--------|---------|
| **Primary** | `curly_teddy_natural` | 10065 × 9918 | Beige PBR fabric: soft, napped curly fleece with shaggy plush pile | beige, fluffy, blanket, fleece, plush, softbox, fuzzy, shaggy | Poly Haven | CC0 |
| **Alt** | `knitted_fleece` | 8103 × 8226 | Brown knitted fleece, dense soft jersey knit, warm wool-like surface | brown, jersey, warm, knitted, fleece, knit, dense weave | Poly Haven | CC0 |

### 3.5 Wood Veneer (Furniture Surfaces)

| Priority | Asset ID | Resolution | Description | Tags | Author | License |
|----------|----------|------------|-------------|------|--------|---------|
| **Primary** | `walnut_veneer` | 16384 × 16384 | Walnut veneer — use for desk, bookcase, chair surfaces | smooth, grain, wood grain, raw, raw wood, veneer, veneer wood, walnut | Poly Haven | CC0 |
| **Alt 1** | `smoked_walnut_veneer` | 8192 × 8192 | Smoked walnut: warm brown tones, fine natural grain, subtle satin sheen | natural grain, grain, walnut, walnut wood, hardwood, natural wood grain, natural wood, brown | Poly Haven | CC0 |
| **Alt 2** | `red_oak_veneer` | 8192 × 8192 | Red oak veneer: light natural wood grain, subtle pores, smooth veneer finish | oak, raw wood, veneer, veneer wood, natural grain, grain, natural wood grain, wood grain | Poly Haven | CC0 |
| **Alt 3** | `teak_veneer` | 8192 × 8192 | Teak veneer: warm brown tones, fine natural grain, subtle knots, smooth | natural grain, grain, natural wood grain, natural wood, natural veneer, natural, teak, brown | Poly Haven | CC0 |

### 3.6 Fabric (Chair Upholstery)

| Priority | Asset ID | Resolution | Description | Tags | Author | License |
|----------|----------|------------|-------------|------|--------|---------|
| **Primary** | `brown_leather` | 8192 × 8192 | Vintage matte brown leather, fine grain, soft wrinkles, creases, subtle wear | leather, brown, chair, seat, upholstery, vintage, matte, wrinkled | Poly Haven | CC0 |
| **Alt 1** | `fabric_leather_01` | 8192 × 8192 | Aged chestnut leather upholstery, fine grain, visible stitching, creased | leather, couch, seat, chair, wrinkled, aged, vintage, textile | Poly Haven | CC0 |
| **Alt 2** | `leather_red_02` | 8192 × 8192 | Smooth red-brown leather, fine pebbled grain, soft creases | leather, even, smooth, new, creased, wrinkled, semi-gloss, upholstery | Poly Haven | CC0 |

---

## 4. 3D Models

### 4.1 Desk

| Priority | Asset ID | Polycount | Dimensions (W×H×D) | Description | Tags | License |
|----------|----------|-----------|-------------------|-------------|------|---------|
| **Primary** | `metal_office_desk` | 6,898 | 2000 × 788 × 947 mm | Metal office desk, worn grey finish, dual pedestal drawers, chrome handles, tapered legs — sturdy industrial look | office, desk | CC0 |
| **Alt 1** | `round_wooden_table_01` | 8,640 | 1399 × 1005 × 1399 mm | Vintage round wooden table, turned pedestal, splayed legs, dark polished finish | vintage, wood, round | CC0 |
| **Alt 2** | `ClassicConsole_01` | 7,566 | 1543 × 949 × 589 mm | Carved Victorian Gothic wooden console, ornate scrollwork, cabriole legs, aged patina | Victorian, Gothic, carved, wood | CC0 |

### 4.2 Chair

| Priority | Asset ID | Polycount | Description | Tags | License |
|----------|----------|-----------|-------------|------|---------|
| **Primary** | `dining_chair_02` | 22,013 | Modern dining chair, tufted brown leather, high cushioned back/seat, dark wooden legs | tufted, leather, dining, modern, cushioned, dark wood | CC0 |
| **Alt 1** | `modern_arm_chair_01` | 8,916 | Modern wooden armchair, warm oak frame, plush black leather cushions, contemporary lounge styling | modern, oak, black leather, armchair, lounge | CC0 |
| **Alt 2** | `ArmChair_01` | 5,626 | Vintage Victorian armchair, varnished carved wood frame, upholstered seat, gothic/classic styling | Victorian, carved, wood, upholstered, gothic, classic | CC0 |

### 4.3 Bookcase / Shelving

| Priority | Asset ID | Polycount | Dimensions (W×H×D) | Description | Tags | License |
|----------|----------|-----------|-------------------|-------------|------|---------|
| **Primary** | `wooden_bookshelf_worn` | 10,106 | 1374 × 2063 × 581 mm | Worn wooden bookshelf, multiple shelves, distressed weathered grain, layered finish | bookshelf, wooden, worn, weathered | CC0 |
| **Alt 1** | `painted_wooden_cabinet_02` | 966 | 997 × 2569 × 729 mm | Rustic vintage bookcase, worn blue paint, aged wood | bookcase, painted, rustic, vintage | CC0 |
| **Alt 2** | `Shelf_01` | 182 | 1003 × 2080 × 257 mm | Simple tall bookshelf, weathered distressed paint, aged dirty finish | shelf, simple, wooden | CC0 |

### 4.4 Decorative Books

| Priority | Asset ID | Polycount | Description | Tags | License |
|----------|----------|-----------|-------------|------|---------|
| **Primary** | `book_encyclopedia_set_01` | 67,306 | Vintage leather-bound encyclopedia set, gold-embossed spines, alphabetic labels, realistic wear | encyclopedia, leather, vintage, gold | CC0 |
| **Alt 1** | `decorative_book_set_01` | 112,560 | Realistic book set, varied paperback and hardcover spines, colors and sizes | books, paperback, hardcover | CC0 |

### 4.5 Desk Lamp

| Priority | Asset ID | Polycount | Dimensions (W×H×D) | Description | Tags | License |
|----------|----------|-----------|-------------------|-------------|------|---------|
| **Primary** | `desk_lamp_arm_01` | 25,710 | 617 × 879 × 408 mm | Industrial articulated desk lamp, clamp-mounted, sprung hinges, vintage adjustable arm | desk, lamp, industrial, articulated, clamp | CC0 |
| **Alt 1** | `vintage_oil_lamp` | 7,208 | 220 × 802 × 220 mm | Victorian oil lamp, brass base, floral porcelain globe, tall glass chimney | Victorian, brass, porcelain, oil lamp | CC0 |
| **Alt 2** | `industrial_pipe_lamp` | 8,626 | 183 × 364 × 262 mm | Industrial pipe lamp, weathered metal, heavy bolted base, exposed filament | industrial, pipe, metal, filament | CC0 |

### 4.6 Potted Plant

| Priority | Asset ID | Polycount | Description | Tags | License |
|----------|----------|-----------|-------------|------|---------|
| **Primary** | `potted_plant_02` | 69,806 | Terracotta pot, weathered rim, textured soil, variegated green healthy leaves | potted, plant, terracotta, variegated, green | CC0 |
| **Alt 1** | `potted_plant_01` | 96,030 | Medium potted plant, lush scalloped green leaves, weathered terracotta pot | potted, plant, lush, scalloped, green | CC0 |
| **Alt 2** | `potted_plant_04` | 6,097 | Small zebra haworthia succulent, weathered ceramic pot | succulent, ceramic, small | CC0 |

---

## 5. Missing Catalog Alternatives

| Item Needed | Poly Haven Status | Fallback Strategy |
|-------------|-------------------|-------------------|
| **Computer monitor** | Not available | Use Godot primitive (Box3D + MeshInstance) with emissive screen material; model not critical for P21-T02 |
| **Modern desk (wooden)** | `metal_office_desk` is metal; wooden alternatives (`round_wooden_table_01`, `ClassicConsole_01`) available but not classic office desks | Use `metal_office_desk` as primary; refine later |
| **Standing lamp** | Not available in standing variant | `industrial_wall_sconce` or `vintage_oil_lamp` as desk accent lamp only |
| **Wall art / paintings** | Not available | Use Godot planes with texture; or create procedurally |
| **Desk accessories** (pen holder, papers, etc.) | `chemistry_set` exists but not study-appropriate | Placeholder geometry for P21-T02; add later |
| **Window frame / curtains** | HDRI may include window elements but no standalone model | Use Godot primitives for window frame |

---

## 6. Download Manifest

All assets are **CC0** (public domain) via Poly Haven. Downloads use:

```
https://www.polyhaven.com/assets/<name>          — web page + download page
https://api.polyhaven.com/assets/<name>/files     — file URLs for direct download
```

### Texture files (for `walnut_veneer`, `beige_wall_001`, etc.)
Each texture provides: **Albedo/Color**, **Roughness**, **Normal**, **Ambient Occlusion** (and sometimes **Metalness**, **Height**) in formats: **JPEG** (8K–32K), **PNG**, **EXR**.

### Model files (for desk, chair, bookcase, etc.)
Each model provides: **GLB** (embedded materials/textures), **FBX**, **OBJ+MTL**, **BLEND**.

### HDRI files
Each HDRI provides: **Equirectangular JPEG**, **EXR** (HDR).

---

## 7. Material Assignment Plan

| Surface | Texture / Model | Maps Needed | Notes |
|---------|----------------|-------------|-------|
| Floor | `walnut_veneer` (or `wooden_floor_01`) | Albedo, Roughness, Normal, AO | Tiled 4×4, anisotropic grain direction |
| Walls | `beige_wall_001` | Albedo, Roughness, Normal, AO | Tiled 2×2, slight color variation per wall |
| Ceiling | `white_plaster_02` | Albedo, Roughness, Normal | Simpler than walls, less AO |
| Bookcase | `wooden_bookshelf_worn` model + `walnut_veneer` texture | Model GLB + texture override | If bookcase has baked textures, override with PBR |
| Desk | `metal_office_desk` model + `walnut_veneer` top | Model GLB + top surface override | Metal body stays original; wood top uses veneer |
| Chair | `dining_chair_02` model + `brown_leather` upholstery | Model GLB + fabric override | Wood legs use `walnut_veneer` |
| Rug | `curly_teddy_natural` texture | Albedo, Roughness, Normal, AO | Plane under desk/chair area |
| Books | `book_encyclopedia_set_01` model | Model GLB (already has textures) | Place on bookcase shelves |
| Lamp | `desk_lamp_arm_01` model | Model GLB (already has textures) | Add PointLight3D with warm color |
| Plant | `potted_plant_02` model | Model GLB (already has textures) | Place on bookshelf or floor corner |

---

## 8. Asset Download Order & Priority

1. **HDRI** (`poly_haven_studio`) — ~500 MB EXR or ~50 MB JPEG — lighting baseline
2. **Floor texture** (`walnut_veneer`) — ~50 MB — foundational surface
3. **Wall texture** (`beige_wall_001`) — ~50 MB — foundational surface
4. **Desk model** (`metal_office_desk`) — ~5 MB GLB — central furniture
5. **Chair model** (`dining_chair_02`) — ~5 MB GLB — seating
6. **Bookcase model** (`wooden_bookshelf_worn`) — ~5 MB GLB — wall furniture
7. **Lamp model** (`desk_lamp_arm_01`) — ~5 MB GLB — accent + light source
8. **Plant model** (`potted_plant_02`) — ~10 MB GLB — decoration
9. **Books model** (`book_encyclopedia_set_01`) — ~10 MB GLB — bookcase detail
10. **Rug texture** (`curly_teddy_natural`) — ~30 MB — floor decoration

**Total estimated download size:** ~770 MB (EXR textures) or ~220 MB (JPEG/optimized)

---

## 9. License Summary

All Poly Haven assets are released under **CC0 1.0 Universal** (public domain). No attribution required but appreciated. See: https://polyhaven.com/license

---

*Manifest produced from live API data at https://api.polyhaven.com/assets — 2382 total assets catalogued, 997 HDRIs, 864 textures, 521 models reviewed.*

## Exploration room reuse (T61, 2026-10-08)

The six new rooms instance the existing verified bookshelf, encyclopedia, chair,
plant and desk-lamp packages, and the existing walnut/plaster photos. Original
room joinery, instruments, sculptures, floor shader and four SVG prints are
documented in [world art](../art/README.md). No downloaded asset, checksum or
provider attribution changed; the broader acquisition gates remain open.
