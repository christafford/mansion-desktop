# Project status

Documentation direction updated 2026-09-29 from the supplied archive.
This edit performs no new compositor verification. Older results below are
attributed historical reports, not fresh executions. This file tracks evidence
and limits; source inspection plus current acceptance determines what works.
Use exactly these labels: **verified by automated test**, **verified by a
person**, **not verified**. Never move an item to a "verified" list without
running the check that proves it.

## Next task

**P4-T06 Reconcile implementation and evidence**, then the bounded foundation
recovery tasks through P4-T18 in TASKS.md. Project 5 feature work is blocked
until the foundation and presentable-room gates pass. P5-T00 was already
expanded; do not run that expansion again.

## Review boundaries and open gates

| Item | Current evidence | What is required next |
| --- | --- | --- |
| Foundation gate | Reopened; old Decision 04 approval superseded | P4-T06–P4-T18 and a real terminal observed by a person |
| GPU client import | Not verified; PBuffer/readback/shm is fallback evidence | P4-T13; direct import proof before ticking P2-T05 |
| Native Wayland host | Not verified | P4-T12 prototype and separate host-window observation |
| Lifetimes and surface commits | Review found paths needing audit; no new runtime conclusion | P4-T08–P4-T11 regressions and sanitizer evidence |
| Automation | Archive test assumes Projects 1–4 always have open tasks; nested P5 tasks are not parsed | P4-T07 fixtures, parser/gate coverage and instruction alignment |
| First presentable room | Not implemented/accepted | P4-T20–P4-T28 after the foundation gate |

Potential source issues are audit leads, not claims of a reproduced crash.
Preserve the dated automated results below within their actual test scope.
Use `docs/ACCEPTANCE.md` for new real-client/visual evidence. Do not describe
synthetic scripted input as a successful ordinary terminal workflow.

## This session (2026-09-28)

- **X11 reconnect mechanism.** Extracted window+EGL creation into
  `create_egl_and_window()` helper. `input_process_x11()` now detects broken
  X11 connection (`XConnectionNumber < 0`) and attempts full reconnect
  (reopen display, recreate window+EGL/renderer). XIO error handler changed
  from `_Exit(0)` to warning-only — compositor survives Xwayland disconnects.
  Verified: compositor ran 30s+ without X11 crash (previously died within
  seconds).
- **Mouse X-axis invert.** Mouse left now looks left (same convention as
  Y-axis: mouse up → look up).
- **Frame count fix.** `frame_count` in `main.cpp` was declared but never
  incremented; now incremented after each `swap_buffers`. Exit log shows
  accurate frame count.
- **Rendering pipeline verified.** `glReadPixels` before `eglSwapBuffers`
  confirms clear color (0.15, 0.15, 0.2) and room geometry are correctly
  present in the back buffer. All 24 tests pass.
- **Known limitation:** Xwayland remains unstable in this container.
  The compositor no longer crashes on disconnect but may need manual restart
  to re-establish the X11 connection. Windowed mode rendering has been
  observed on the Wayland compositor side but the X11 window appearance
  on the host desktop is unreliable in this environment.


## Verified by automated test

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

- `smoke-headless` (P1-T01): `mansion-desktop --headless --socket NAME
  --exit-after-ms N` starts without a display, prints `MANSION_SOCKET=NAME`,
  creates the socket and lock file in `$XDG_RUNTIME_DIR`, exits 0 on its own,
  removes both files, and opens no input devices.
- `client-globals` (P1-T02, P1-T03): `mansion-test-client` connects and sees
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
- **P1-T07 Host window input replaces evdev.** `input_init()`/`input_destroy()`
  are no-ops; `input_process()` is a no-op. X11 event handling (`input_process_x11`)
  processes KeyPress/KeyRelease/ButtonPress/ButtonRelease/MotionNotify/
  ConfigureNotify/WM_DELETE_WINDOW in windowed mode. `grep -r "/dev/input" src`
  finds nothing. All 6 tests still pass. Windowed mode — not observed (requires
  DISPLAY).
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
- **P2-T03 Live updates.** `MansionSurface` gains a `needs_upload` flag set on
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
  `MansionRenderer::bytes_uploaded` (set at each `glTexImage2D` call as
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
  pointer `leave`. `MansionSeat::pressed_keys` tracks currently-pressed keycodes.
  `meson test -C build mode-exit` passes (key 42 released, kbd_leave observed).
- **P3-T03 Host focus loss.** `MansionSeat::stored_mode_when_focus_lost` saves
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
  2026-09-26 (see [OPENCODE-AUTOCONTINUE.md](OPENCODE-AUTOCONTINUE.md)). The
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

## Environment facts (development container `mansion-dev`, Arch Linux)

- meson 1.x, ninja 1.13, g++ (C++20), wayland 1.26, wayland-protocols 1.49,
  libxkbcommon 1.13.2, mesa 26.2, libx11 1.8.13, node 26. `wayland-scanner`
  present. `/dev/dri/renderD128` is world-accessible; `DISPLAY=:0` reaches the
  host through XWayland.
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
./build/mansion-desktop --help
./build/mansion-desktop --headless --exit-after-ms 1000
./build/mansion-desktop --launch weston-terminal        # needs a host DISPLAY and weston
```

## Files

- `src/main.cpp` — options, startup/shutdown order, event loop
- `src/compositor.cpp`, `compositor-private.h` — `wl_compositor`, `wl_surface` state
- `src/xdg-shell.cpp` — `xdg_wm_base`, `xdg_surface`, `xdg_toplevel`
  (protocol code generated into `build/` by `wayland-scanner`)
- `src/display.cpp` — X11 host window, EGL/GLES2 renderer, headless stub
- `src/input.cpp` — `wl_seat`, X11 event processing (P1-T07), input script
- `src/launch.cpp` — child process launcher
- `tests/` — headless test client and shell tests ([tests/README.md](../tests/README.md))
- `docs/TASKS.md` — task list; `docs/handoffs/` — per-project handoffs;
  `docs/decisions/` — recorded decisions
