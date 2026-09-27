# Project status

Updated 2026-09-26. This file is the single source of truth for what works.
Use exactly these labels: **verified by automated test**, **verified by a
person**, **not verified**. Never move an item to a "verified" list without
running the check that proves it.

## Next task

**P2-T04 Texture lifetime.** Textures are created on first commit,
resized on buffer size change, deleted on surface destruction. Check
`glGetError()` after each frame in debug builds and log once per error.

## Verified by automated test

Run `meson test -C build --print-errorlogs`.

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
- Auto-continue plugin: `node --test .opencode/tests/*.test.js` (49 tests),
  plus one live run against OpenCode 2.0.16 with the local model on
  2026-09-26 (see [OPENCODE-AUTOCONTINUE.md](OPENCODE-AUTOCONTINUE.md)). The
  first two live runs ended early: the model answered `AUTOCONTINUE_DONE` after
  one follow-up with 29 tasks open, and a plugin reload dropped the in-memory
  run. Both are now covered by tests (`AUTOCONTINUE_DONE` is checked against
  `docs/TASKS.md`; run state persists in `.opencode/auto-continue.state.json`).
  The hardened plugin has not yet been exercised in a live OpenCode run.

## Verified by a person

Nothing yet. No real Wayland client has ever opened a window on this
compositor. `weston-terminal` is not installed in the development container
(`sudo pacman -S weston` provides it). P1-T10 not observed (human task).

## Not verified / known broken

- **xdg-shell is minimal.** `xdg_wm_base` v3 with `xdg_surface`/`xdg_toplevel`
  only: one fixed 800x600 configure, `ack_configure` serial stored but not
  checked, `set_window_geometry`/title/app_id/min/max ignored, `xdg_positioner`
  accepted and ignored, `get_popup` posts a protocol error. No `ping` is sent.
  Resize configures on host window resize (P1-T09), popups in Project 5.
- **Seat handles one client badly.** A second `get_keyboard` posts a protocol
  error; `wl_seat` binds overwrite each other. Fixed in P1-T06-A (multi-seat
  lists). Proper keymap (xkbcommon), repeat_info, modifiers, keyboard enter/leave,
  pointer enter/leave/motion with hit testing in P1-T06-B through P1-T06-F.
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
