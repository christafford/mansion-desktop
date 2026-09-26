# Task list

This is the executable form of [PROJECT-ROADMAP.md](../PROJECT-ROADMAP.md).
Autonomous sessions work from this file: take the first unchecked task whose
prerequisites are checked, finish it, prove it, tick it, commit.

## Conventions

- One task per turn. A task is small enough to finish, build, and test in one
  sitting. If it is not, split it (add sub-tasks here) instead of doing half.
- Every task has an **Acceptance** line. It is a command, or a short list of
  commands, that must pass with no display attached. Run it before ticking.
- Tick a task by changing `- [ ]` to `- [x]` and appending the commit hash or
  date, for example `- [x] P1-T01 ... (a1b2c3d)`.
- Tasks marked **(human)** need a person at the screen. Autonomous sessions do
  not perform them, do not tick them, and do not claim them. They write the
  procedure and leave the result as "not observed" in `docs/STATUS.md`.
- Commit after each verified task: `git add -A && git commit -q -m "<id>: <summary>"`.
  Never push. Never rewrite history.
- Build and test commands are always:

  ```sh
  meson setup build --buildtype=debug        # first time only
  meson compile -C build
  meson test -C build --print-errorlogs
  ```

- `tests/` holds the test client, shell/python test scripts, and expected
  outputs. Tests run the compositor with `--headless` and talk to it through
  its Wayland socket. See `tests/README.md`.
- Never weaken or delete a test to make it pass. Fix the code, or record a
  precise blocker in `docs/STATUS.md` and stop the task.
- Do not add dependencies beyond those in `meson.build` without recording a
  decision in `docs/decisions/`.

Recorded decisions that constrain these tasks: [decisions/01-stack.md](decisions/01-stack.md),
[decisions/02-nested-io-and-verification.md](decisions/02-nested-io-and-verification.md).

---

## Project 1: nested compositor foundation

Goal (from the roadmap): a native Wayland terminal displays in 2D inside the
host window, accepts typing and pointer input, resizes, and can close without
killing Mansion. Automated tests use `tests/mansion-test-client`; the terminal
itself is the human acceptance step.

- [x] **P1-T01 Headless harness and socket handling.** Add CLI options
  `--headless` (no X11 window, no EGL required, no host input), `--socket NAME`,
  `--exit-after-ms N`. Default socket name `mansion-<pid>`; never inherit the
  host `WAYLAND_DISPLAY`. Print `MANSION_SOCKET=<name>` on stdout once the
  socket is listening. Remove the socket and lock file on exit. Fix the
  dangling `c_str()` pointer in `main.cpp`. Add `tests/smoke_headless.sh` and a
  `meson test` entry.
  **Acceptance:** `meson test -C build smoke-headless` passes: the compositor
  starts headless, the socket appears in `$XDG_RUNTIME_DIR`, the process exits 0
  after the timeout, and the socket file is gone.

- [x] **P1-T02 Test client: globals.** Add `tests/mansion-test-client.c` (C,
  `wayland-client`) built by Meson. `--socket NAME` connects; it prints one line
  `global <interface> <version>` per advertised global and exits 0. Add
  `tests/client_globals.sh` that starts the compositor headless and asserts
  `wl_compositor`, `wl_shm`, and `wl_seat` are advertised.
  **Acceptance:** `meson test -C build client-globals` passes.

- [ ] **P1-T03 xdg-shell replaces wl_shell.** Generate `xdg-shell` server and
  client code with `wayland-scanner` in `meson.build` (protocol XML from
  `pkg-config --variable=pkgdatadir wayland-protocols`). Implement
  `xdg_wm_base` (`get_xdg_surface`, `create_positioner` may post an error for
  now, `pong`), `xdg_surface` (`get_toplevel`, `ack_configure`,
  `set_window_geometry`), `xdg_toplevel` (`set_title`, `set_app_id`,
  `set_min_size`, `set_max_size`, ignore the rest safely). On the first commit
  without a buffer send `xdg_toplevel.configure` (800x600, states empty) then
  `xdg_surface.configure` with a serial. Delete `src/shell.cpp`/`shell.h` and
  the `wl_shell` global. Extend the test client with `--toplevel`: create a
  toplevel, wait for configure, print `configure <w> <h>`, ack it.
  **Acceptance:** `meson test -C build client-toplevel` passes (configure line
  received with non-zero size); `client-globals` now sees `xdg_wm_base`.

- [ ] **P1-T04 wl_shm buffers, commit, release, frame callbacks.** Track pending
  and current buffer per surface. On commit: read `wl_shm_buffer` size and
  format, store width/height, keep a reference to the buffer resource, release
  the previous buffer with `wl_buffer.release`, and handle buffer destruction
  (`wl_resource_add_destroy_listener`). Implement `wl_surface.frame`: callbacks
  are sent `done` with a millisecond timestamp once per render tick, then
  destroyed. Support `wl_compositor.create_region` minimally (accept and
  ignore). Test client `--buffer WxH --color RRGGBB` creates an shm pool,
  fills the buffer, attaches, damages, commits, requests a frame callback, and
  prints `frame <time>` and `release` when they arrive.
  **Acceptance:** `meson test -C build client-frame` passes (both lines within
  2 seconds).

- [ ] **P1-T05 Offscreen rendering and screenshots.** In headless mode create an
  EGL display with `EGL_PLATFORM_SURFACELESS_MESA` (or `EGL_DEFAULT_DISPLAY`)
  and a pbuffer surface; if EGL is unavailable, keep running without a renderer
  and still fire frame callbacks. Fix the renderer to map pixel coordinates to
  clip space, upload `ARGB8888`/`XRGB8888` shm buffers with `glTexImage2D`
  (BGRA if `GL_EXT_texture_format_BGRA8888`, otherwise swizzle on the CPU), and
  draw surfaces at their position. Add `--screenshot PATH` writing a binary PPM
  of the framebuffer after the next rendered frame. Add `tests/ppm_pixel.py
  FILE X Y RRGGBB [tolerance]`.
  **Acceptance:** `meson test -C build render-shm` passes: a 200x100 red client
  buffer at the origin produces red at pixel (100,50) and the clear colour at
  (600,500).

- [ ] **P1-T06 Seat objects done properly.** Support several `wl_seat` binds and
  several `wl_keyboard`/`wl_pointer` objects per seat (keep lists, remove on
  destroy). Send `keymap` (xkbcommon, `xkb_keymap_new_from_names` with
  `XKB_DEFAULT_*`, fallback `us`), `repeat_info` (seat v4+), `modifiers`, and
  keyboard `enter` (with the pressed-keys array) / `leave`. Keyboard focus goes
  to the most recently mapped toplevel. Pointer `enter`/`leave`/`motion`
  use a hit test against surface rectangles. Maintain one increasing serial.
  Add `--input-script FILE` to the compositor: lines `wait MS`,
  `key CODE press|release`, `motion X Y`, `button CODE press|release`,
  `focus lost|gained`, `quit`. Test client `--report-input` prints
  `kbd_enter`, `kbd_leave`, `key CODE STATE`, `ptr_enter`, `ptr_leave`,
  `motion X Y`, `button CODE STATE`.
  **Acceptance:** `meson test -C build input-routing` passes: scripted key
  and pointer events reach a mapped client in the expected order, and a second
  client receives nothing while the first is focused.

- [ ] **P1-T07 Host window input replaces evdev.** In windowed mode select
  `KeyPress|KeyRelease|ButtonPress|ButtonRelease|PointerMotion|FocusChange|
  StructureNotify` on the X11 window, handle `WM_DELETE_WINDOW`, translate X
  keycodes (minus 8) and buttons (1→BTN_LEFT 0x110, 2→BTN_MIDDLE 0x112,
  3→BTN_RIGHT 0x111, 4/5→vertical axis) into the same internal event path used
  by `--input-script`. `ConfigureNotify` updates the viewport. Remove all
  `/dev/input` code. Windowed mode is exercised by a person; headless tests
  guard the shared path.
  **Acceptance:** `meson test -C build` still passes; `grep -r "/dev/input" src`
  finds nothing; `./build/mansion-desktop --exit-after-ms 500` exits 0 with
  `DISPLAY` set and prints no "input device" lines.

- [ ] **P1-T08 Launcher: child lifecycle.** Already done in `src/launch.cpp`:
  children inherit the environment with `WAYLAND_DISPLAY=<our socket>`,
  `DISPLAY` unset, `GDK_BACKEND`/`QT_QPA_PLATFORM=wayland`, `XDG_RUNTIME_DIR`
  unchanged; whitespace argv split; repeatable `--launch`; SIGTERM then SIGKILL
  on exit. Remaining: reap children with `SIGCHLD` (self-pipe or `signalfd`
  added to the Wayland event loop with `wl_event_loop_add_fd`) and log
  `child <pid> exited <status>`; log `client connected`/`client disconnected`
  with the client pid from `wl_client_get_credentials` (use
  `wl_display_add_client_created_listener`).
  **Acceptance:** `meson test -C build launch-client` passes: the compositor
  launches `tests/mansion-test-client --toplevel --buffer 64x64 --exit-after-ms 300`
  headless, logs its connection and exit, and exits 0 itself.

- [ ] **P1-T09 Lifecycle robustness.** Client disconnect while mapped removes the
  surface, textures, focus, and pending callbacks without use-after-free. Two
  toplevels stack (later on top). Host window resize sends a new configure to
  the focused toplevel. Build once with `-Db_sanitize=address,undefined` in a
  separate `build-asan` directory and run the whole test suite.
  **Acceptance:** `meson test -C build lifecycle` passes (three connect/
  disconnect cycles, then a fresh client still gets `configure`), and
  `meson test -C build-asan` reports no sanitizer errors.

- [ ] **P1-T10 (human) Terminal smoke test.** Install `weston` (provides
  `weston-terminal`). Run `./build/mansion-desktop --launch weston-terminal`
  from a host session with `DISPLAY` set. Type, click, resize the host window,
  close the terminal from its own UI, confirm Mansion keeps running, quit
  Mansion, confirm the host desktop is fine. Record the result and
  versions in `docs/STATUS.md` under "Verified by a person". Write the
  procedure in `docs/ACCEPTANCE.md` (autonomous sessions write the procedure;
  a person performs it).

- [ ] **P1-T11 Project 1 wrap-up.** Reconcile `docs/STATUS.md`, finish
  `docs/handoffs/01.md` (what changed, exact commands, evidence, limitations,
  the first Project 2 task), and update `README.md`/`GETTING_STARTED.md` if
  commands changed.
  **Acceptance:** all Project 1 automated tasks ticked; `meson test -C build`
  passes; docs list P1-T10 as observed or not observed truthfully.

---

## Project 2: live application surface in 3D

Depends on Project 1 automated tasks (P1-T10 may still be unobserved).

- [ ] **P2-T01 Math module.** `src/math.h` with `vec3`, `mat4`, `perspective`,
  `look_at`, `translate`, `rotate_y/x`, `multiply`, `transform_point`. Header
  only, no dependency. Unit test executable `tests/test_math.cpp`.
  **Acceptance:** `meson test -C build math` passes (projection of known points
  matches expected values within 1e-4).

- [ ] **P2-T02 Perspective panel.** Add an MVP uniform to the shader. Introduce a
  `Camera` (position, yaw, pitch) and a `Panel` (position, size, normal).
  Draw the focused surface's texture on the panel; keep the 2D path behind
  `--flat`. Add `--camera X,Y,Z,YAW,PITCH` for tests. `tests/project_point.py`
  computes where a world point lands on screen with the same math.
  **Acceptance:** `meson test -C build render-panel` passes: with a fixed
  camera, the client colour is present at the projected panel centre and the
  clear colour at a pixel outside the projected quad.

- [ ] **P2-T03 Live updates.** Re-upload textures only for surfaces committed
  since the last frame (track damage per surface). Frame callbacks keep
  flowing while the panel is shown.
  **Acceptance:** `meson test -C build render-update` passes: client commits
  red, screenshot A; commits blue, screenshot B; A is red and B is blue at the
  projected centre.

- [ ] **P2-T04 Texture lifetime.** Textures are created on first commit,
  resized on buffer size change, deleted on surface destruction. Check
  `glGetError()` after each frame in debug builds and log once per error.
  **Acceptance:** `meson test -C build render-lifecycle` passes (three
  connect/draw/disconnect cycles, last screenshot valid, no GL error lines).

- [ ] **P2-T05 Accelerated client experiment (Decision gate A input).** Add a
  test client mode `--egl` using `wayland-egl` that clears to a colour and
  swaps. Import its buffers with `EGL_WL_bind_wayland_display`
  (`eglBindWaylandDisplayWL`, `eglQueryWaylandBufferWL`, `eglCreateImageKHR`)
  or `zwp_linux_dmabuf_v1`. If the container has no usable GPU path, record
  the exact failure (extension list, error codes) in
  `docs/decisions/03-accelerated-buffers.md` and mark the task blocked, not
  done.
  **Acceptance:** `meson test -C build render-egl` passes, or the decision
  record exists with evidence and `docs/STATUS.md` lists the blocker.

- [ ] **P2-T06 Camera movement.** WASD/arrow keys move, mouse look with the
  right button held (windowed mode). The same actions are available to
  `--input-script` through `key`/`motion`.
  **Acceptance:** `meson test -C build camera-move` passes: a scripted forward
  move changes the projected panel size between two screenshots.

- [ ] **P2-T07 Frame timing.** `--stats` prints per-frame render time and bytes
  uploaded every 60 frames; record numbers for the shm path in the handoff.
  **Acceptance:** `meson test -C build` passes; `docs/handoffs/02.md` contains
  measured numbers with the machine description.

- [ ] **P2-T08 Project 2 wrap-up and gate A record.** Handoff, status, decision
  on whether to continue with the current renderer/compositor integration.
  **Acceptance:** docs updated; every Project 2 task ticked or recorded as
  blocked with evidence.

---

## Project 3: world/application input state machine

- [ ] **P3-T01 Explicit modes.** `enum class InputMode { World, Application }`
  with one owner. World mode: input drives the camera, clients get nothing.
  Application mode: input goes to the focused surface. `--input-script` gains
  `mode world|app`.
  **Acceptance:** `meson test -C build mode-routing` passes.

- [ ] **P3-T02 Reserved shortcut and clean exit from application mode.**
  Configurable `--world-key` (default `F12`, evdev code 88) returns to world
  mode. Leaving application mode releases held keys (send `key` release for
  every pressed key), sends `modifiers`, keyboard `leave`, pointer `leave`.
  **Acceptance:** `meson test -C build mode-exit` passes (held key released,
  leave events observed).

- [ ] **P3-T03 Host focus loss.** `focus lost` (X `FocusOut`) behaves like
  leaving application mode without changing the stored mode; `focus gained`
  re-enters the client only if the mode is still Application.
  **Acceptance:** `meson test -C build focus-loss` passes.

- [ ] **P3-T04 Full-size presentation.** In application mode the focused
  surface is drawn 2D, scaled to fit the host window, and configured to the
  host size; returning to world mode restores the panel and the previous size.
  **Acceptance:** `meson test -C build present-fullsize` passes (client
  receives the new configure; screenshot is filled with the client colour).

- [ ] **P3-T05 Targeting and selection.** Ray from the screen centre against
  the panel; `--input-script` `key 28 press` (Enter) on a targeted panel enters
  application mode.
  **Acceptance:** `meson test -C build select-panel` passes.

- [ ] **P3-T06 Client exit during application mode.** Return to world mode,
  clear focus, no dangling pointers.
  **Acceptance:** `meson test -C build app-exit` passes under the ASan build.

- [ ] **P3-T07 Project 3 wrap-up.** Handoff 03, status, documented host
  shortcuts that cannot be captured.

---

## Project 4: one-room end-to-end prototype

- [ ] **P4-T01 Room geometry and collision.** Floor, four walls, a desk box,
  and a monitor frame as coloured, flat-shaded meshes; AABB collision keeps the
  camera inside the room and above the floor.
  **Acceptance:** `meson test -C build room-render` passes (floor colour at
  the bottom centre pixel, wall colour at the top centre pixel);
  `room-collision` passes (scripted walk into a wall stops at the bound).

- [ ] **P4-T02 Monitor slot and teleport.** The launched client's panel sits in
  the monitor frame. Key `T` (evdev 20) teleports to a stored viewpoint facing
  the monitor (reduced-motion access).
  **Acceptance:** `meson test -C build teleport` passes.

- [ ] **P4-T03 Milestone 1 scripted demonstration.** `tests/milestone1.sh`
  runs the nine concept steps headless: start, walk, launched client mapped
  on the monitor, approach, select, application mode, type (client reports the
  keys), return to world with the reserved key, client still connected and
  drawing.
  **Acceptance:** `meson test -C build milestone1` passes.

- [ ] **P4-T04 (human) Milestone 1 with a real terminal.** Same nine steps
  with `weston-terminal` in a host session. Record observations in
  `docs/STATUS.md`.

- [ ] **P4-T05 Project 4 wrap-up and Decision gate A.** Handoff 04, status,
  and an explicit gate decision in `docs/decisions/`.

---

## Projects 5–8: useful persistent workspace

Each project starts with an expansion task. Autonomous sessions perform the
expansion task, then work the resulting list.

- [ ] **P5-T00 Expand Project 5** (multiple windows and application lifecycle)
  into tasks of the same shape as above: window registry, several toplevels,
  `xdg_popup` and positioner, transient/dialog association by client, manual
  artifact assignment, focus switching by keyboard, tests for each.
- [ ] **P6-T00 Expand Project 6** (durable artifact model, SQLite schema,
  slot-based movement, restart persistence, migration and interrupted-write
  tests). Adding SQLite requires a decision record.
- [ ] **P7-T00 Expand Project 7** (launch recipes, controlled restoration,
  failure and duplicate handling).
- [ ] **P8-T00 Expand Project 8** (search, keyboard selection, teleportation,
  usability trial notes).

Projects 9 and later are defined in the roadmap only; expand them when their
dependencies are ticked.
