# P21-T10: Compositor Core Extraction — Handoff

> Historical report, superseded by the [2026-10-02 audit](11-godot-reality-audit.md).
> Do not use this file's completion/blocker claims to select work. T00/T01/T02/T03/T10
> are reopened. The core still depends on the renderer; model dependencies are
> missing; the extension is not registered or load-verified. A local Godot binary
> and display are available. The `user://project.godot`/GUI-registration claim is
> incorrect. Follow [current tasks](../TASKS.md) and [status](../STATUS.md).

## Summary

Separated the reusable Wayland protocol/client/seat core from host GL rendering
and input. The compositor core (`compositor.cpp`, `compositor-private.h`,
`xdg-shell.cpp`) no longer depends on GL/EGL at all.

## Changes

### New file
- **`src/renderer-surface.h`** — Rendering-layer per-surface state (`MansionRendererSurface`
  with `GLuint gl_texture`). Only included by presentation-layer code.

### Modified files
- **`src/compositor-private.h`** — Removed `#include <GLES2/gl2.h>` and
  `GLuint gl_texture` field from `MansionSurface`. The struct now contains
  only Wayland protocol state.
- **`src/display.h`** — Added `#include "renderer-surface.h"` and four
  accessor function declarations.
- **`src/display.cpp`** — Added static `unordered_map` mapping protocol
  surface `wl_resource*` → `MansionRendererSurface*`, a global pointer
  (`g_current_display`) for surface-creation-time access, and four
  accessor implementations:
  - `get_renderer_surface()` — lookup by resource pointer
  - `ensure_renderer_surface()` — lazy-allocate on first access
  - `set_renderer_surface_texture()` — store GL texture (deletes old)
  - `destroy_renderer_surface()` — free texture and remove mapping
- **`src/compositor.cpp`** — Replaced all direct `gl_texture` field accesses
  with accessor calls. Surface creation calls `ensure_renderer_surface()`.

## Layer boundaries

```
compositor.cpp ──→ protocol objects, surface state, frame callbacks
compositor-private.h ──→ MansionSurface (Wayland-only), MansionCompositor
renderer-surface.h ──→ MansionRendererSurface (GL texture only)
display.cpp ──→ presentation: EGL, rendering, host window, seat input
display.h ──→ accessor API that bridges protocol ↔ presentation
main.cpp ──→ integration: creates compositor, display, input, launches clients
```

## What the GDExtension bridge (P21-T11) needs

The bridge needs these without pulling in EGL/rendering:

1. **Start/stop** — `create_compositor()` / `destroy_compositor()`
2. **Non-blocking pump** — `wl_event_loop_dispatch(event_loop, 0)`
3. **Surface enumeration** — iterate `compositor->surface_list`
4. **Surface state access** — width, height, buffer_resource, needs_upload,
   damage (all in `MansionSurface`, no GL deps)
5. **Frame callback firing** — `wl_list_for_each_safe` over
   `surface->frame_callback_list`
6. **Seat config** — seat is created in `create_seat()` (input.cpp)
7. **Clean shutdown** — `destroy_compositor()` tears down protocol and surfaces

The bridge should **not** include:
- `display.h` (pulls in GL via renderer-surface.h, though `GLuint` is
  trivially forward-declarable — better to declare only what's needed)
- `room.h` (room rendering)
- Input device scanning

## Key design decisions

- **Global pointer (`g_current_display`)** — Used only in `ensure_renderer_surface()`
  to allocate renderer surfaces during surface creation in compositor.cpp.
  Cleared in `destroy_display()`. Safe because there is only one display
  per compositor instance.

- **Static `unordered_map`** — The surface→renderer-surface mapping lives
  in a file-local static function to avoid global state leakage.

- **Lazy allocation** — `ensure_renderer_surface()` uses `emplace()` so
  the map only grows for surfaces that are actually rendered.

## Verification

- `meson compile -C build` — passes
- `meson test -C build --print-errorlogs` — 28/28 pass
- `meson compile -C build-asan && meson test -C build-asan --print-errorlogs`
  — 28/28 pass, zero sanitizer errors

## Open questions

- Should `display.h` forward-declare `GLuint` instead of including
  `renderer-surface.h`, to avoid GL transitively in the protocol header?
  Currently `display.h` includes `renderer-surface.h` which includes GL.
  Since `display.h` is the presentation layer, this is acceptable.
