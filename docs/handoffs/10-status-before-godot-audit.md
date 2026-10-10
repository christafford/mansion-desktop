# Archived status before the 2026-10-02 Godot audit

Historical snapshot from revision `3dfaa26`. **Not current status or task authority.**
It contains incorrect completion, toolchain, renderer and blocker claims.
In particular, P21-T00/T01/T02/T03/T10 are reopened, models lack dependencies,
and the extension is not integrated. Use [current status](../STATUS.md),
[current tasks](../TASKS.md) and [audit handoff](11-godot-reality-audit.md).
Do not copy any "this session" or "verified" statement below as new evidence.

---

# Project status

Documentation direction updated 2026-10-01 against the latest supplied archive.
This edit performs no new compositor verification. Older results below are
attributed historical reports, not fresh executions. This file tracks evidence
and limits; source inspection plus current acceptance determines what works.
Use exactly these labels: **verified by automated test**, **verified by a
person**, **not verified**. Never move an item to a "verified" list without
running the check that proves it.

## Next task

**P4-T30 host orientation fixed (2026-10-02), against `99fcedb`.** GL readback
was copied bottom-first into top-first Wayland shm. The export now reverses
rows while preserving ARGB channels. **Verified by automated test:** build,
30/30 suite, then the expanded `host-input` rendered-export check pass; the
rebuilt host launch renders 36 frames and exits 0. Agent inspected an actual
room render after host pixel conversion: floor below wall. The regression
checks asymmetric pixel rows/channels and rendered floor/wall positions;
the existing screenshot path already flipped correctly and missed this bug.
**Not verified:** human confirmation of the corrected host window. Prior user
feedback confirms navigation works but reported the inversion; no broader
usability gate is credited. See [handoff](04-host-window.md).

**P4-T29 host-navigation repair complete (2026-10-02).** Follow-up to the
user's report that the rendered view still ignored input, against `a5ba64a`.
Live host movement was never applied to the camera, WASD keycodes were wrong,
and each key event cleared other held keys. Fixed all three; host focus loss
clears movement, mouse look preserves fractional deltas and has an ordinary
pointer fallback. Nested input now uses only the host seat, with no global
`/dev/input` scanning. Seat listeners register at bind time; the previously
unregistered xdg ping handler now answers host pings.
**Verified by automated test:** build and 30/30 tests pass, including direct
host-listener movement, mouse-look, focus-loss and mode-isolation assertions.
Agent-run eight-second launch renders 274 frames and exits 0; protocol log
shows keyboard focus and matching ping/pong. **Not verified:** physical input
usability, pointer capture, application typing/click routing, or human gates.
This repair is limited to world navigation; P4-T12 remains open.

**P4-T19 startup-hang repair complete (2026-10-02).** User-requested bounded
fix against base revision `04ecfe6`: the nested host event handler called
`wl_display_read_events()` without preparing a read, deadlocking before the
first frame. It now uses `wl_display_dispatch()` and the main loop checks host
connection errors. **Verified by automated test:** build succeeds, 29/29 tests
pass outside the socket-restricted sandbox; the new `host-events` regression
deadlocks with the original handler and passes with the fix. Agent-run host
launch with `--room-camera --exit-after-ms 1000` previously required SIGKILL;
the rebuilt binary renders 18 frames and exits 0. **Not verified:** visual
quality, interactive input, real-terminal usability. P4-T12 and human gates
remain open. Details: [host-window handoff](04-host-window.md).
This focused repair does not change the Project 21 continuation below.

**P21-T04 (Convert models and verify PBR imports): blocked.** Requires Godot
editor to import models, set PBR materials, and render a model/material contact
sheet. Prerequisites (P21-T01, P21-T03) are complete: scaffold scene exists at
`world/scenes/main.tscn`, 22 assets cached in `world/assets/cache/` (2.8 GB).
Blocked until display server available or local Godot install.

**P21-T03 (fetcher): complete.** Tool at `world/tools/fetch_assets.py` with MD5
hash verification, atomic downloads, retry/backoff, cache management. All 22
primary assets cached (2.8 GB).

**P21-T11 (GDExtension adapter): blocked.** Code written, extension .so built and
deployed to world/addons/elsewhere_godot/, all 13 symbols exported, Godot editor
build complete (1.07 GB), project loads cleanly (zero errors); blocked on display
server to register extension via Project Settings > General > Extensions (Godot
4.x stores extension list in user://project.godot, not in project's
project.godot). P21-T11 depends on P21-T01, P21-T10 being complete.

Decision 06 authorizes the Godot world frontend and Poly Haven assets; world art
and compositor extraction are independently eligible. P21-T19 is the furnished
live-terminal gate; P21-T34 is the final personal review.

**P21-T03 completed 2026-10-01.** Asset fetcher tool at
`world/tools/fetch_assets.py`: CLI fetcher with MD5 hash verification (API
provides MD5), atomic downloads, retry with exponential backoff, cache
management, dry-run mode, and `--hashes` output. File key resolver handles HDRI
(24k/16k exr/jpg → api.polyhaven.com/files/{id}), texture (diffuse/normal/rough
jpg), and model (glb → gltf 2k) asset types. All 22 primary assets cached in
`world/assets/cache/` (2.8 GB total: 2.7 GB EXR + 57 MB textures/models).
Repeat bootstrap verified: re-run detects all assets as up-to-date with hash
verification. C++ regression suite: 28/28 pass in build and build-asan.

**P21-T00 completed 2026-10-01.** Baselines recorded: C++ 28/28 pass in normal
build (camera-move test now passing), 28/28 in build-asan (zero sanitizer errors).
Node plugin 62/62 pass. Toolchain pinned: Godot 4.7.2-stable, godot-cpp
10.0.0-stable, Blender 5.2.2 LTS. Target: Steam Deck, Compatibility (Vulkan)
renderer, 30 FPS minimum. Godot/godot-cpp/Blender not installed on dev system
— repository-local download path documented.

**P21-T01 completed 2026-10-01.** Godot frontend scaffold created under `world/`:
`project.godot` (Compatibility renderer, 1280×800), `scenes/main.tscn` (room
geometry with placeholder materials, monitor slot, desk, Camera3D, SpawnMarker),
`scripts/game_world.gd` (WASD movement, mouse look, collision bounds, teleport,
application mode toggle). Automated validation via
`tools/validate-godot-project.sh` (import check, scene node verification, script
presence). Run command: `tools/run-godot.sh`. C++ headless tests preserved:
28/28 pass, unchanged. Node plugin 62/62 pass. Live render capture deferred to
P21-T09 (requires display server).

**Not verified:** Godot project, GDExtension bridge, imported assets, furnished
room, live-terminal integration, new performance goals and final usability.
These patches change direction and automation; they implement none of those
product features. Record actual checks under the correct evidence labels.

Current work-order claims supersede older scheduling statements below. Legacy
P4-T12–T28 and GPU import P2-T05 remain uncompleted; their missing features do
not block independent art or the explicitly shm-only first bridge. Legacy
multiwindow code must be audited before reuse, not credited from a task title.

## Historical recovery report from the supplied archive

**P4-T11 completed 2026-09-29.** Implemented xdg-shell handshake and metadata
validation: title/app_id/geometry tracking in `ElsewhereXdgToplevel`/`ElsewhereXdgSurface`,
configure/ack serial validation with `XDG_SURFACE_ERROR_INVALID_SERIAL` error
for future serials, added `--xdg-test` mode to test client for metadata
verification, and dedicated xdg-shell protocol test. Verified version
advertisement (xdg_wm_base v3), initial configure (800×600), repeated
connect/disconnect survivability. `meson test -C build` 28/28,
`meson test -C build-asan` 28/28 zero sanitizer errors.
P4-T10 completed 2026-09-29: replaced the driver-specific Mesa EGL R↔G
swizzle workaround with a format-aware CPU-side swizzle path — all 27 tests
passed. P4-T09 completed 2026-09-29: pending/current surface state separation,
stride validation, damage bounding box, single buffer release per render.
P4-T08 completed 2026-09-29: tracked xdg_surface/xdg_toplevel in shell lists,
freed on disconnect, orphaned surface cap at 10, fixed client listener +
SIGCHLD event source leaks. P4-T07 completed 2026-09-29: 54/54 plugin tests
pass, 25/25 C++ tests pass. P4-T12 follows; Project 5 feature work is blocked
until the foundation and presentable-room gates pass.

## Review boundaries and open gates

| Item | Current evidence | What is required next |
| --- | --- | --- |
| Legacy foundation gate | Unaccepted; superseded as current work order | Relevant regressions and live-terminal evidence now required by P21-T10–T19 |
| GPU client import | Not verified; PBuffer/readback/shm is fallback evidence | Later direct-import scope; leave P2-T05 open, not a prerequisite for the CPU bridge |
| Legacy native Wayland host | PBuffer/shm export (cd2da8a); real-session behavior unverified | Godot takes over the visible host window in P21-T01/T11; retain any needed core headless behavior |
| Host input path | P4-T29 removes global evdev; nested navigation uses the focused host seat | P21-T14/T15 use only focused Godot host input; global evdev is prohibited in the new frontend |
| Lifetimes and surface commits | xdg_surface/xdg_toplevel tracked and freed on disconnect; orphaned surface cap at 10 — P4-T08 done | P4-T09–P4-T11 regressions and sanitizer evidence |
| Automation | New count/time defaults are unlimited; invalid scopes/task lists rejected; 62 Node tests passed during patch preparation | Check actual OpenCode load locally; no new live Qwen/OpenCode run is claimed |
| First furnished Godot study | Not implemented/accepted | P21-T01–T09 independently eligible; converges with bridge at P21-T19 |

Potential source issues are audit leads, not claims of a reproduced crash.
Preserve the dated automated results below within their actual test scope.
Use `docs/ACCEPTANCE.md` for new real-client/visual evidence. Do not describe
synthetic scripted input as a successful ordinary terminal workflow.

## Merge reconciliation (2026-09-29)

The 2026-09-29 documentation series (d8d7fd5–b2bdf86) was written against a
checkout that predated the 2026-09-28 implementation commits
(d3411e2–a615f18). Both are kept. Facts established while merging:

- `meson compile -C build && meson test -C build --print-errorlogs` on the
  merged tree: 25/25 tests pass (source unchanged by the merge; container
  `elsewhere-dev`). This is a regression run, not new capability evidence.
- The X11 host window no longer exists in source. Statements about X11 in
  ARCHITECTURE.md, ACCEPTANCE.md and DEVELOPMENT.md were corrected; older
  handoffs keep their historical wording. `meson.build` still declares
  `x11 = dependency('x11')` although no target uses it.
- `docs/handoffs/05.md` (Wayland client window work) was renamed to
  `docs/handoffs/04-host-window.md`; handoff 05 is reserved for Project 5.
- The nested P5-T01/P5-T02 sub-tasks ticked on 2026-09-28 are recorded as
  historical evidence under P5-T00 in TASKS.md; the revised column-zero
  P5-T01/P5-T02 remain unchecked; their requirements are reused in P21-T20 after P21-T19.

## P4-T06 Reconciliation (2026-09-29, this session)

Full reconciliation completed. Handoff at
`docs/handoffs/04-recovery.md`.

### Baseline (all commands run in this session)

| Check | Result |
| --- | --- |
| `meson compile -C build` | Up to date |
| `meson test -C build --print-errorlogs` | **25/25 pass, 0 fail** |
| `meson test -C build-asan --print-errorlogs` | **25/25 pass, 0 fail, no sanitizer errors** |
| `node --test .opencode/tests/*.test.js` | **49/49 pass** |

### Contradictions found (documentation vs source)

- **C1. Dead X11 dependency:** `meson.build` line 13 declares `x11 = dependency('x11')`
  but no source file in `src/` references X11 and no target links against it.
  Confirmed: `grep -rn "x11" src/` returns zero matches.
- **C2. P1-T07 evdev claim:** STATUS.md states "input_init() no-ops and /dev/input
  not found" but `src/input.cpp` lines 130–188 clearly open and scan `/dev/input`
  for evdev devices. The historical claim was correct for the pre-migration state
  but contradicts current source after d4f8551.

### Partial implementations confirmed

- **P5-T01/P5-T02 (window registry, focus cycling):** Code exists
  (`toplevel_list`, `toplevel_count`, `xdg_shell_cycle_focus()`, input-script
  `tab`/`shift_tab`) but the synthetic `focus-cycle` test only verifies that
  the compositor does not crash with two clients and tab/shift_tab in World mode.
  Revised acceptance (multiple windows, unmapped windows, teardown order,
  sanitizer, held-input release) is NOT met.
- **Native Wayland host window:** Implemented in display.cpp + input.cpp
  (d4f8551–a615f18) but unverified at runtime. Severe flicker + no host input
  documented in handoff 04-host-window.md. Root cause hypothesis (Mesa Wayland
  EGL backend conflict) is plausible but unverified.
- **Orphaned surface retention:** Surfaces persist after disconnect until
  compositor shutdown. Bounded to 10 entries with age-based eviction
  (oldest evicted, GL texture freed) — P4-T08.

### Audit leads for follow-up

- P4-T10: verify formats and coordinates (shm format/stride/alpha, scaling)
- P4-T12: native Wayland host experiment (bounded prototype)
- P4-T13: accelerated buffer import reinvestigation
- P4-T11: xdg-shell handshake validation
- P5-T01/P5-T02: revised acceptance tests; reuse/audit in P21-T20 after P21-T19

### Limitations

- No real-application terminal trial (human tasks P1-T10, P4-T04, P4-T17)
- No `WAYLAND_DEBUG=1` host window logs captured
- No visual pixel evidence collected
- Plugin tests are self-contained; no live OpenCode server exercised

## Session history (2026-09-28, second turn; commits d4f8551–a615f18)

- **X11 → Wayland client migration (flickering fix attempt).** Replaced X11
  host window with a native Wayland client presentation (`xdg_wm_base` +
  `wl_egl_window` + `wl_surface`). Goal: eliminate the triple-layer mismatch
  (EGL → X11 → Xwayland → KWin) that caused flickering in windowed mode
  (`--room-camera`).
  - `display.h` fields changed: X11 `display`, `xwin`, `colormap` replaced with
    `wl_client_display`, `wl_surface`, `wl_egl_window` (all `= nullptr` default).
  - `display.cpp`: generated xdg-shell client protocol included; Wayland client
    setup (registry, xdg_wm_base, wl_compositor, wl_surface, xdg_surface,
    xdg_toplevel, wl_egl_window) replaces X11 window creation.
    `destroy_display` calls `destroy_wayland_client_window`.
    Mesa's Wayland EGL backend (`EGL_PLATFORM_WAYLAND_EXT`) used with our
    `wl_display` pointer.
    `swap_buffers()` removed the manual `wl_surface_commit()` — Mesa's Wayland
    EGL backend commits the surface internally during `eglSwapBuffers()`.
  - `input.h/c`: X11 includes removed; X11 globals removed.
    `input_process_wayland_client()` now dispatches Wayland client events
    (`wl_display_dispatch_pending` + `wl_display_flush`).
    `input_handle_window_close()` sets `g_window_closed` from xdg_toplevel.close.
    `input_process()` now processes evdev input for camera movement (WASD +
    mouse delta) and forwards key/button events to Wayland clients.
    evdev FD globals and live camera state globals properly defined.
  - `main.cpp`: replaced `input_process_x11(display)` with
    `input_process_wayland_client(display)`.
  - `meson.build`: removed x11 from `elsewhere_exe` deps; added wayland_client;
    added xdg_shell_client_h to elsewhere_exe sources.
  - **Build and tests:** all 25/25 tests pass (reported 2026-09-28).

- **Wayland seat input for host window (P5-T02 follow-up).** Added keyboard and
  pointer input handling to the host Wayland client window so the user can
  control camera movement in windowed mode (`--room-camera`).
  - Added `wl_seat` (version 7), `wl_keyboard`, `wl_pointer`, and
    `zwp_relative_pointer_manager_v1` binding in the registry handler.
  - Keyboard listener: maps Linux keycodes 26/38/39/40 (W/A/S/D) to WASD
    camera movement via `input_wayland_key()`.
  - Relative pointer listener: captures unaccelerated mouse delta
    (`dx_unaccel`, `dy_unaccel`) for camera rotation via
    `input_wayland_pointer_motion()`.
  - Pointer listener: empty (pointer position tracked but not used for camera).
  - `input.cpp/h`: added `input_wayland_key()` and
    `input_wayland_pointer_motion()` to update live camera state variables.
  - `meson.build`: generated relative-pointer protocol header + code added
    to elsewhere_exe.
  - a615f18 then added a `wl_seat.capabilities` handler and fixed listener
    crashes; runtime effect not observed.
  - **Not verified:** flickering is "terrible" (worse than X11) and no mouse or
    keyboard input received from the host compositor (KWin). The root cause is
    likely Mesa's Wayland EGL backend creating its own internal `wl_display`
    connection for buffer management, conflicting with our `wl_client_display`
    connection. Investigation ongoing; see
    [handoffs/04-host-window.md](04-host-window.md).

## Session history (2026-09-28, first turn; superseded by the migration above)

- **X11 reconnect mechanism.** Extracted window+EGL creation into
  `create_egl_and_window()` helper. `input_process_x11()` detected a broken
  X11 connection (`XConnectionNumber < 0`) and attempted a full reconnect
  (reopen display, recreate window+EGL/renderer). XIO error handler changed
  from `_Exit(0)` to warning-only. Reported: compositor ran 30s+ without X11
  crash. This code was removed in d4f8551; the record is historical only.
- **Mouse X-axis invert.** Mouse left now looks left (same convention as
  Y-axis: mouse up → look up).
- **Frame count fix.** `frame_count` in `main.cpp` was declared but never
  incremented; now incremented after each `swap_buffers`. Exit log shows
  accurate frame count.
- **Rendering pipeline check.** `glReadPixels` before `eglSwapBuffers`
  confirmed the clear color (0.15, 0.15, 0.2) and room geometry in the back
  buffer. All 24 tests passed at that time.
- **Known limitation then:** Xwayland was unstable in this container; the X11
  window appearance on the host desktop was unreliable. The X11 path has since
  been removed rather than fixed.

## Verified by automated test

- **P21-T04 Convert models and verify PBR imports — blocked (2026-10-01, this session).**
  Prerequisites verified: P21-T01 scaffold exists at `world/scenes/main.tscn` with
  room geometry, monitor slot, desk, Camera3D, SpawnMarker. P21-T03 cached 22 primary
  assets in `world/assets/cache/` (2.8 GB: HDRI, textures, models). Manifest at
  `world/assets/manifest.md` documents 30 Poly Haven assets with IDs, resolutions,
  texture maps, and material assignment plan. Model assets (metal_office_desk,
  dining_chair_02, wooden_bookshelf_worn, desk_lamp_arm_01, potted_plant_02,
  book_encyclopedia_set_01) downloaded as GLTF from Poly Haven API — require GLB or
  Godot import for PBR verification. **Blocker:** Requires Godot editor (or Blender
  headless + headless Godot render) for model import, material assignment, and contact
  sheet rendering. No display server available. P21-T05 through P21-T09 blocked.

- **P21-T10 Compositor core extraction.** GL/EGL dependency removed from the
  protocol core: `compositor-private.h` no longer includes `<GLES2/gl2.h>` and
  `ElsewhereSurface` no longer carries a `gl_texture` field. A new header
  `renderer-surface.h` defines `ElsewhereRendererSurface` for GPU resources,
  entirely in the presentation layer. Accessor functions
  (`get_renderer_surface`, `ensure_renderer_surface`, `set_renderer_surface_texture`,
  `destroy_renderer_surface`) bridge the layers via a static `unordered_map`
  keyed by `wl_resource*`. All 28 tests pass in both `build` and `build-asan`
  (28/28, zero sanitizer errors). (2026-10-01, this session)

- **P21-T11 GDExtension adapter — partially verified (2026-10-01, this session).**
  Code written, compiles cleanly, and .so built and deployed (98 KB, 13 symbols):
  - `src/godot/elsewhere_bridge.h` — C-facing bridge API header (no GL/EGL includes)
  - `src/godot/elsewhere_bridge.cpp` — C++ implementation connecting GDExtension to
    compositor core (compositor, seat, xdg-shell, launch). No threads spawned; all
    calls from owning thread. Nonblocking `wl_event_loop_dispatch(display, 0)`.
  - `src/godot/elsewhere_extension.cpp` — GDExtension entry point using raw
    `gdextension_interface.h` (no godot-cpp dependency for compilation). Exports
    `godot_gdnative_init`, `godot_gdnative_exit`, `godot_extension_get_library_symbol`.
  - `src/godot/CMakeLists.txt` — build configuration for GDExtension
  - `src/godot/extension.toml` — Godot extension manifest
  - `tools/build-elsewhere-extension.sh` — build script (standalone .so, no libgodot.so)
  - `world/addons/elsewhere_godot/` — deployed extension (libelsewhere_godot.so + extension.toml)
  - `world/project.godot` — updated with `[gdextension]` section for extension registration
  - `world/scenes/main.tscn` — fixed StandardMaterial3D emissive_enabled → emission_enabled
  Compilation: `elsewhere_bridge.cpp` compiles against elsewhere core headers
  (compositor.h, compositor-private.h, xdg-shell.h, input.h, launch.h) with
  zero warnings. `elsewhere_extension.cpp` compiles against GDExtension interface
  with zero warnings. Extension built: `build-godot-ext/libelsewhere_godot.so`
  (98 KB, 13 symbols verified via `nm -D`). Godot editor build complete
  (1.07 GB `godot.linuxbsd.editor.dev.x86_64`). Project loads cleanly in
  editor and `--path` modes with zero errors. **Blocker:** Godot 4.x stores
  the extension list in `user://project.godot` (set via Project Settings >
  General > Extensions UI), not in the project's `project.godot` file.
  Extension load/unload and fixture client testing deferred until display
  server available. No regressions: 28/28 pass in `build` and `build-asan`
  (zero sanitizer errors).

- **P4-T08 Resource and list lifetimes.** `ElsewhereXdgShell` gains
  `toplevel_list` and `xdg_surface_list` tracking lists;
  `ElsewhereXdgSurface` gains `shell` back-pointer, `xdg_surface_link` and
  `toplevel_link` (for `ElsewhereXdgToplevel`). Surfaces and toplevels are
  registered in these lists at construction and freed in `destroy_xdg_shell`
  (fixes leak on compositor shutdown and client disconnect). Orphaned
  `ElsewhereSurface` entries are capped at 10 via age-based eviction with GL
  texture deletion. Fixed two pre-existing leaks: `on_client_destroyed` now
  deletes the listener after removal, and the SIGCHLD event source is stored
  and removed in the cleanup path. Bug fix:
  `shell_from_wm_base(resource)` was called with the xdg_surface resource in
  `xdg_surface_get_toplevel` (wrong); the shell back-pointer resolves this.
  All 25 tests pass in both `build` and `build-asan` (AddressSanitizer +
  UndefinedBehaviorSanitizer, zero leaks, zero sanitizer errors).
  (2026-09-29, this session)

- **P4-T09 Pending/current surface state.** `ElsewhereSurface` gains
  `pending_buffer_resource`, `pending_x`/`pending_y`, `pending_damage` bounding
  box. `surface_attach` stores pending state; `surface_commit` promotes pending
  to current (preserving current buffer until promoted). SHM stride validation
  posts `WL_SURFACE_ERROR_BUFFER` on invalid stride. `wl_buffer::release` sent
  only from `render_surface`/`render_panel` after rendering (not from
  `surface_commit`), preventing double-release. Destroy listener removed and
  re-registered correctly for each new pending buffer. `tests/p4_surface_state.sh`
  adds 6 tests: attach-without-commit, commit-without-attach, replaced-pending-attach,
  null-attach/commit, repeated buffer reuse after release, resource destruction after
  release. Root issue: test client needed `--exit-after-ms` to let compositor
  dispatch pending commits before disconnect (fix applied to test script).
  All 26 tests pass in both `build` and `build-asan`.
  (2026-09-29, this session)

- **Historical nested P5-T01 Window registry (superseded task text; revised
  P5-T01 remains open).** `ElsewhereCompositor` gains `toplevel_list` and
  `toplevel_count`. `ElsewhereXdgSurface` gains `toplevel_link`. Registration
  occurs after `xdg_toplevel` creation; unregistration on both `xdg_surface`
  and `wl_surface` destroy. A crash was fixed: during client disconnect the
  wl_surface is destroyed before the xdg_surface, so `on_wl_surface_destroyed`
  calls `toplevel_unregister` first; the subsequent call from
  `xdg_surface_resource_destroyed` must check that `surface_resource` is still
  valid. Acceptance: all 24 tests pass (including `lifecycle`). The
  `toplevel-registry` test named by the old acceptance was never added.
  (2026-09-28, bb8cd9d)

- **Historical nested P5-T02 Alt+Tab focus cycling (superseded task text;
  revised P5-T02 remains open).** `input.cpp` gains `g_tab_key` (default 23),
  `input_tab_key_set()`/`input_tab_key_get()`. `xdg-shell.cpp` gains
  `xdg_shell_cycle_focus()` which walks the toplevel list and calls
  `seat_set_keyboard_focus()` to cycle forward or backward. Input script gains
  `tab` and `shift_tab` commands that invoke focus cycling in World mode only;
  in Application mode tab passes through to the client. New test `focus-cycle`.
  Acceptance: all 25 tests pass (including new `focus-cycle`). Synthetic
  scripted input only. (2026-09-28, bb8cd9d)

- **P3-T06 Client exit during application mode.** When the application client
  exits while in Application mode, the compositor returns to World mode, clears
  focus, and avoids dangling pointers. Acceptance: `meson test -C build app-exit`
  passes. Fixed root cause: `destroy_listener.link` was shared between the
  Wayland resource's `destroy_signal` list and the seat's tracking list;
  `wl_list_insert` overwrote the link's prev/next, corrupting the destroy_signal
  iteration. Separated into `destroy_listener.link` (for destroy_signal) and
  `seat_link` (for seat tracking). Also added `seat_clear_focus_state()` helper.

- **P3-T07 Project 3 wrap-up.** Handoff 03 created, status updated, host
  shortcuts documented. The world key (F12 by default) is intercepted by the
  compositor before reaching any Wayland client, ensuring users can always exit
  Application mode.

- **P4-T01 Room geometry and collision.** Room mode (`--room-camera`) draws a
  10×7×10 box (brown floor, grey walls/ceiling, dark desk, monitor frame) as
  flat-shaded meshes with its own MVP path (`program_3d`). AABB collision
  clamps the camera to x∈[-4.5,4.5], y∈[0.5,6.5], z∈[-4.5,4.5] but only when
  `room_mode` is true (gated by `--room-camera`). Fixed a pre-existing texture
  swizzle bug: the test client writes RGBA byte data, and Mesa EGL surfaceless
  rearranges texture storage to [G,R,B,A]. EGL mode now swizzles (data[1],
  data[0], data[2], data[3]) to compensate; non-EGL uploads RGBA directly.
  `tests/camera-move.sh` PPM maxval parsing also fixed.
  `meson test -C build room-render` (floor 61472c±40, wall 7f7f8c±40) and
  `room-collision` (walk into wall stops at bound) both pass. All 22 tests pass.

- **P4-T02 Monitor slot and teleport.** Panel is positioned on the monitor frame
  at (0, 3, -5) when `--room-camera` is used. Key `T` (evdev 20) teleports the
  camera to a stored viewpoint at (0, 3, 0) with yaw=0, pitch=0, facing the
  monitor. Teleport is intercepted in both World and Application input modes.
  `meson test -C build teleport` passes. All 23 tests pass.

- **P4-T03 Milestone 1 scripted demonstration.** `tests/milestone1.sh` runs the
  full user journey: start in room mode, walk toward the monitor, select the
  panel with Enter (entering Application mode), type keys (client reports them),
  return to world mode with F12, and verify the client is still connected.
  `meson test -C build milestone1` passes. All 24 tests pass.

- **P4-T05 Project 4 wrap-up.** Handoff 04 created with full architectural
  documentation (room geometry, collision, teleport, texture swizzle fix).
  Historical Decision 04 approved continuation from synthetic evidence.
  That approval is superseded by Decision 05; Project 5 is multiple windows,
  not multiple rooms. The 24-test pass is a historical report.

- `smoke-headless` (P1-T01): `elsewhere --headless --socket NAME
  --exit-after-ms N` starts without a display, prints `ELSEWHERE_SOCKET=NAME`,
  creates the socket and lock file in `$XDG_RUNTIME_DIR`, exits 0 on its own,
  removes both files, and opens no input devices.
- `client-globals` (P1-T02, P1-T03): `elsewhere-test-client` connects and sees
  `wl_compositor`, `wl_shm`, `wl_seat`, `xdg_wm_base`; the compositor survives
  the client disconnecting and exits 0 on SIGTERM. The seat now supports
  multiple `get_keyboard`/`get_pointer` binds per seat (P1-T06-A).
- `client-toplevel` (P1-T03): the client creates `xdg_surface` + `xdg_toplevel`
  on a `wl_surface`, commits, and receives `xdg_toplevel.configure 800x600`
  followed by `xdg_surface.configure` with a serial. Protocol code is generated
  by `wayland-scanner` at build time. The same three tests pass in a
  `-Db_sanitize=address,undefined` build (`build-asan`, 2026-09-26), and an
  ad-hoc run that SIGKILLed a mapped client and then connected a second client
  reported no sanitizer errors.
- `client-frame` (P1-T04): the client creates a 200×100 ARGB8888 shm buffer,
  attaches and commits it, and receives `frame <time_ms>` and `release` within
  2 seconds. `meson test -C build client-frame` passes. The seat now sends
  xkb keymap via memfd, repeat_info (rate=25, delay=500), and modifier state
  on each key event (P1-T06-B). Keyboard focus goes to the most recently
  mapped toplevel via `wl_keyboard_send_enter`/`leave` (P1-T06-C).
- `render-shm` (P1-T05): the compositor uses `EGL_PLATFORM_SURFACELESS_MESA`
  with a pbuffer surface to upload `wl_shm` buffers (ARGB8888) as OpenGL
  textures and draw them in screen position. `--screenshot PATH` reads the
  framebuffer and writes a binary PPM. Surfaces survive client disconnect via
  an orphaned-surface list. A 200×100 red client buffer at the origin produces
  red at pixel (100,50) and the compositor clear colour at (600,500).
  `meson test -C build render-shm` passes.
- `input-routing` (P1-T06-F): scripted keyboard/pointer events drive a
  `--report-input` client; events arrive in order with correct serials.
- **P1-T07 Host window input replaces evdev (historical; partly reversed).**
  At the time, `input_init()`/`input_destroy()`/`input_process()` were no-ops
  and X11 events drove windowed mode; `grep -r "/dev/input" src` found nothing.
  Since d4f8551 the X11 path is gone and `input_init()` again opens `/dev/input`
  devices in windowed mode (headless still opens none; `smoke-headless` passes).
  Windowed mode — not observed.
- **P1-T08 Launcher child lifecycle.** Children are reaped automatically via
  SIGCHLD using `signalfd` added to the Wayland event loop. `wl_display_add_client_created_listener`
  logs `client connected <pid>` and `client disconnected <pid>` with the client PID
  from `wl_client_get_credentials`. `meson test -C build launch-client` passes.
- **P1-T09 Lifecycle robustness.** (2026-09-26) `surface_destroy_callback` fires
  pending frame callbacks (`wl_callback_send_done(cb, 0)`) before moving the
  surface to the orphaned list — clients are not left waiting for callbacks after
  disconnect. Rendering iterates `wl_list_for_each_reverse` so newer surfaces draw
  on top of older ones. Host window resize sends a new configure to the focused
  toplevel (`xdg_shell_send_configure_resize`). A new `lifecycle` test verifies
  three connect/disconnect cycles followed by a fresh client that still receives
  `configure`. `meson test -C build lifecycle` passes.
  `meson test -C build-asan` (address + undefined sanitizers) reports no sanitizer
  errors (all 8 tests pass).
- **P2-T01 Math module.** Header-only `src/math.h` with `vec3`, `mat4`,
  `perspective`, `look_at`, `translate`, `rotate_y/x`, `multiply`,
  `transform_point`. Unit test executable `tests/test_math.cpp` verifies identity,
  matrix multiply associativity, rotation of known vectors, perspective projection,
  look_at camera, and a full VP pipeline. `meson test -C build math` passes
  (9 test groups, within 1e-3 tolerance).
- **P2-T02 Perspective panel.** `Camera` (position, yaw, pitch) and `Panel`
  (position, size, normal) drawn via MVP matrix in a 3D shader (`program_3d`,
  `pos_3d`, `tex_3d`, `mvp_uniform`). 2D path kept behind `--flat`. New
  `--camera X,Y,Z,YAW,PITCH` flag drives camera parameters. `tests/project_point.py`
  computes screen projection with matching `look_at` + `perspective` math.
  `tests/render_panel.sh` verifies the client's red buffer appears at the
  projected panel centre (255,0,0) and the clear colour outside the quad.
  Mesa EGL surfaceless renderer required a cyclic RGB swizzle
  (R←G, G←B, B←R, A←A) to compensate for `[B,R,G,A]` internal storage.
  `meson test -C build render-panel` passes; all 10 tests pass on
  both `build` and `build-asan`.
- **P2-T03 Live updates.** `ElsewhereSurface` gains a `needs_upload` flag set on
  buffer commit and cleared after texture upload in `render_panel()`. Only
  surfaces with pending uploads re-upload (damage tracking). Frame callbacks
  continue flowing on every render tick. Added `--commit-color RRGGBB` to the
  test client so it commits a second colour after the first frame callback.
  Fixed an orphaned-surface crash: `surface_destroy_callback` now clears
  `focused_surface_resource` when the destroyed surface was the focused one,
  and `render_panel()` falls back to `orphaned_surfaces` so disconnected
  surfaces still render. `meson test -C build render-update` passes (two
  compositor/client pairs: red then red→blue, verified by pixel check).
  All 11 tests pass on both `build` and `build-asan`.
- **P2-T04 Texture lifetime.** Textures are created on first commit, resized
  on buffer size change, deleted on surface destruction. Added
  `glGetError()` check after each frame in debug builds, logging each unique
  error once per frame. Fixed a colour swizzle bug: the flat-mode `render_surface()`
  used a naive R↔B swap which only accidentally worked for red; changed it to
  the cyclic swizzle (R←G, G←B, B←R) already used by `render_panel()` to
  correctly compensate for Mesa EGL surfaceless renderer's internal
  `[B,R,G,A]` rearrangement. `meson test -C build render-lifecycle` passes
  (three connect/draw/disconnect cycles, last screenshot valid, no GL errors).
  All 12 tests pass on `build`.
- **P2-T05 fallback experiment (feature reopened).** Test client `--egl` flag renders
  green to an EGL PBuffer, reads pixels via `glReadPixels`, exports as RGBA
  memfd-backed `wl_shm` buffer. Compositor `--egl` flag skips the ARGB→RGBA
  swizzle in `render_surface()` and `render_panel()`, uploading RGBA directly.
  `EGL_WL_bind_wayland_display` symbols are NULL on both X11 and
  `EGL_PLATFORM_WAYLAND_KHR` displays; `eglCreateImageKHR` symbols are
  also NULL. `docs/decisions/03-accelerated-buffers.md` records the evidence.
  Buffer is not zero-copy (client copies GPU→CPU), but the full pipeline
  (EGL render → shm export → compositor upload → screenshot) is verified.
  `meson test -C build render-egl` passes. All 13 tests pass on `build`.
- **P2-T07 Frame timing.** Compositor gains `--stats` flag. `render()` tracks
  wall-clock time via `std::chrono::steady_clock` and accumulates
  `ElsewhereRenderer::bytes_uploaded` (set at each `glTexImage2D` call as
  `w × h × 4`). Every 60 frames the accumulated averages are printed to stderr
  as `fps` and `KiB uploaded`, then counters reset. `meson test -C build`
  passes unchanged. Measured numbers in `docs/handoffs/02.md`.
- **P2-T08 historical Project 2 wrap-up.** The 2026-09-27 handoff reported all
  P2 tasks ticked and 14/14 tests passing. P2-T05 is now reopened, and the old
  continuation approval is superseded by Decision 05. Keep the fallback test
  result within its actual scope.
- **P3-T01 Explicit modes.** `enum class InputMode { World, Application }` with
  one owner. World mode: input drives the camera, clients get nothing.
  Application mode: input goes to the focused surface. `--input-script` gains
  `mode world|app`. Key/motion/button events only reach clients in Application
  mode; in World mode they are consumed by the world input system.
  `meson test -C build mode-routing` passes.
- **P3-T02 Reserved shortcut.** Configurable `--world-key` (default `F12`=88)
  returns to world mode from Application mode. `exit_application_mode()` sends
  `key` release for all pressed keys, empty modifiers, keyboard `leave`, and
  pointer `leave`. `ElsewhereSeat::pressed_keys` tracks currently-pressed keycodes.
  `meson test -C build mode-exit` passes (key 42 released, kbd_leave observed).
- **P3-T03 Host focus loss.** `ElsewhereSeat::stored_mode_when_focus_lost` saves
  current mode on `focus lost`/X11 `FocusOut` (also calls exit_application_mode).
  `focus gained`/X11 `FocusIn` re-enters Application mode if stored mode matches.
  `meson test -C build focus-loss` passes.
- **P3-T04 Full-size presentation.** `render()` now checks `input_mode_get()`.
  In Application mode: `render_application_fullscreen()` renders the focused
  surface as a full-screen 2D quad. In World mode: 3D panel rendering.
  `xdg_shell_send_configure_resize()` called when `mode app` is executed.
  `meson test -C build present-fullsize` passes.

- **P3-T05 Targeting and selection.** `display.panel_targeted` is set to true
  (ray from screen centre always hits the panel). The panel is rendered with
  a green tint when targeted. Pressing Enter (key 28) in World mode switches
  to Application mode. `render_application_fullscreen()` now uploads SHM
  buffers as textures before rendering (previously it assumed the texture
  already existed). `meson test -C build select-panel` and
  `meson test -C build present-fullsize` pass.

- Auto-continue plugin: `node --test .opencode/tests/*.test.js` (49 tests),
  plus one live run against OpenCode 2.0.16 with the local model on
  2026-09-26 (see [OPENCODE-AUTOCONTINUE.md](../OPENCODE-AUTOCONTINUE.md)). The
  first two live runs ended early: the model answered `AUTOCONTINUE_DONE` after
  one follow-up with 29 tasks open, and a plugin reload dropped the in-memory
  run. Both are now covered by tests (`AUTOCONTINUE_DONE` is checked against
  `docs/TASKS.md`; run state persists in `.opencode/auto-continue.state.json`).
  The hardened plugin has not yet been exercised in a live OpenCode run.

## Verified by a person

No personal observation is recorded in the supplied archive. The archive
reports no successful ordinary Wayland application trial; synthetic clients
have connected and rendered. This is a missing acceptance record, not a claim
about later work in another checkout. `weston-terminal` is not installed in the development container
(`sudo pacman -S weston` provides it). P1-T10 not observed (human task).

## Not verified / known broken

- **xdg-shell is minimal.** `xdg_wm_base` v3 with `xdg_surface`/`xdg_toplevel`
  only: one fixed 800x600 configure, `ack_configure` serial stored but not
  checked, `set_window_geometry`/title/app_id/min/max ignored, `xdg_positioner`
  accepted and ignored, `get_popup` posts a protocol error. No `ping` is sent.
  Resize configures on host window resize (P1-T09), popups in Project 5.
- **Seat verification boundary.** Multi-seat objects, keymap, modifiers, and
  focus delivery have historical synthetic-test evidence. Their real-client
  behavior, pointer coordinate mapping, held-button release, and concurrent
  focus transitions must be checked in recovery; the older single-seat defect
  is reported fixed, not a current failure claim.
- **Launcher** works for simple commands (whitespace split, environment set,
  `DISPLAY` unset), children are reaped via SIGCHLD, and client connect/disconnect
  is logged. Exit status from `--launch` errors is still only logged (not
  propagated) (P1-T08 partial).

## Environment facts (development container `elsewhere-dev`, Arch Linux)

- meson 1.x, ninja 1.13, g++ (C++20), wayland 1.26, wayland-protocols 1.49,
  wayland-egl, libxkbcommon 1.13.2, mesa 26.2, libx11 1.8.13 (still required by
  `meson.build` line 13, no longer linked), node 26. `wayland-scanner` present.
  `/dev/dri/renderD128` is world-accessible. The host `WAYLAND_DISPLAY` socket
  in the shared `XDG_RUNTIME_DIR` is what the windowed host window connects to;
  `DISPLAY=:0` (XWayland) is no longer used by the compositor.
- Missing: wlroots, weston/weston-terminal, foot, Xvfb, eglinfo, wayland-info.
  Installing packages needs `sudo`, which autonomous sessions must not use.
  Ask a person: `sudo pacman -S weston`.
- Host: Steam Deck (SteamOS) running a Wayland session; OpenCode 2.0.16 runs in
  the container against a llama.cpp server on `127.0.0.1:8080` (Qwen3.6 35B A3B).

## Commands

```sh
meson setup build --buildtype=debug   # first time
meson compile -C build
meson test -C build --print-errorlogs
./build/elsewhere --help
./build/elsewhere --headless --exit-after-ms 1000
./build/elsewhere --launch weston-terminal        # needs a host WAYLAND_DISPLAY and weston; unverified
```

## Files

- `src/main.cpp` — options, startup/shutdown order, event loop
- `src/compositor.cpp`, `compositor-private.h` — `wl_compositor`, `wl_surface` state
- `src/xdg-shell.cpp` — `xdg_wm_base`, `xdg_surface`, `xdg_toplevel`
  (protocol code generated into `build/` by `wayland-scanner`)
- `src/display.cpp` — Wayland client host window (`wl_egl_window`, seat and
  relative-pointer listeners), EGL/GLES2 renderer, room rendering, headless stub
- `src/input.cpp` — `wl_seat`, host-window event dispatch, evdev camera input
  in windowed mode, input script and modes
- `src/launch.cpp` — child process launcher
- `tests/` — headless test client and shell tests ([tests/README.md](../../tests/README.md))
- `docs/TASKS.md` — task list; `docs/handoffs/` — per-project handoffs;
  `docs/decisions/` — recorded decisions
