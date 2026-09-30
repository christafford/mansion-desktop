# Task list

This is the executable form of [PROJECT-ROADMAP.md](../PROJECT-ROADMAP.md).
Autonomous sessions work from this file: take the first eligible unchecked
task in scope, prove its acceptance, record the evidence, and commit.

## Current execution order (2026-09-29 review)

1. Foundation recovery: P4-T06 through P4-T18.
2. First presentable room: P4-T20 through P4-T28.
3. Resume Projects 5–8, then the dependency-ordered roadmap.

P4-T18 and P4-T28 are real product gates. Project 5 expansion already exists,
but its feature tasks are blocked until both gates pass. Keep earlier automated
work and checkmarks as historical records; the old P4-T05 decision does not
authorize new development past these gates. P2-T05 is reopened for direct GPU
buffer import and is investigated by P4-T13; a readback/shm test cannot tick it.

Recovery/visual phases use numeric P4 IDs because the bundled continuation
parser does not understand P4R/P4V. Keep executable checklist items at column
zero; the current parser ignores indented tasks. Use explicit bounded ranges.
Completion of a range is not acceptance of the product or a later gate.

## Conventions

- One task per turn. A task is small enough to finish, build, and test in one
  sitting. If it is not, split it (add sub-tasks here) instead of doing half.
- Every task has an **Acceptance** line and explicit prerequisites for new
  recovery tasks. Behavioral changes require appropriate automated checks;
  graphical/product acceptance also requires observation where stated.
  Documentation/research tasks accept reviewable evidence, not invented tests.
- Distinguish implementation, automated checks, real-client integration, visual
  observation, and product gate decisions. Record command, date, revision,
  environment, expected/actual result, and remaining limits in STATUS/handoff.
- A completed investigation with a blocker does not complete the feature it
  investigated. Keep that feature unchecked and its compatibility limitation
  visible. A gate must explicitly decide whether a constrained fallback is
  sufficient for the next phase.
- Tick a task by changing `- [ ]` to `- [x]` and appending the commit hash or
  date, for example `- [x] P1-T01 ... (a1b2c3d)`.
- Tasks marked **(human)** need a person at the screen. Autonomous sessions do
  not perform them, do not tick them, and do not claim them. They write the
  procedure and leave the result as "not observed" in `docs/STATUS.md`.
- Commit verified task changes only: `git add -- <task files>`, then
  `git commit -q -m "<id>: <summary>"`. Never push or rewrite history.
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

- [x] **P1-T03 xdg-shell replaces wl_shell.** (2026-09-26) Generate `xdg-shell` server and
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

- [x] **P1-T04 wl_shm buffers, commit, release, frame callbacks.** Track pending
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

- [x] **P1-T05 Offscreen rendering and screenshots.** In headless mode create an
  EGL display with `EGL_PLATFORM_SURFACELESS_MESA` and a pbuffer surface; if EGL
  is unavailable, keep running without a renderer and still fire frame callbacks.
  Fix the renderer to map pixel coordinates to clip space, upload `ARGB8888`/
  `XRGB8888` shm buffers with `glTexImage2D` (BGRA if
  `GL_EXT_texture_format_BGRA8888`, otherwise swizzle on the CPU), and draw
  surfaces at their position. Add `--screenshot PATH` writing a binary PPM of
  the framebuffer after the next rendered frame. Keep surfaces alive after client
  disconnect via an orphaned-surface list so the screenshot captures the last
  rendered frame. Add `tests/ppm_pixel.py FILE X Y RRGGBB [tolerance]`.
  **Acceptance:** `meson test -C build render-shm` passes: a 200x100 red client
  buffer at the origin produces red at pixel (100,50) and the clear colour at
  (600,500). (2026-09-26)

- [x] **P1-T06 Seat objects done properly.** Support several `wl_seat` binds and
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
  client receives nothing while the first is focused. (2026-09-27)

- [x] **P1-T06-A Multi-seat objects (lists).** Replace the single `keyboard`
  and `pointer` resources in `MansionSeat` with wl_lists of per-client
  resources. `get_keyboard`/`get_pointer` create new resources; destroy
  listeners clean them up. No `WL_SEAT_ERROR_MISSING_CAPABILITY` error for
  repeated binds.
  **Acceptance:** `meson test -C build client-globals` passes and the
  compositor survives a client that opens two keyboards on the same seat.
  (2026-09-26)

- [x] **P1-T06-B Keymap, repeat_info, modifiers.** Use `xkb_keymap_new_from_names`
  with `XKB_DEFAULT_*` (fallback `us`), send the keymap via memfd, compile
  xkbstate, send `repeat_info` for seat v4+, include modifiers in key events.
  **Acceptance:** `meson test -C build client-frame` passes; headless compositor
  logs no xkb errors. (2026-09-26)

- [x] **P1-T06-C Keyboard enter/leave + focus.** Send keyboard `enter` (with
  the damaged-surface's wl_resource in the regions array) and `leave` events.
  Track keyboard focus — the most recently mapped toplevel wins.
  **Acceptance:** `meson test -C build input-keyboard` passes (client reports
  `kbd_enter`/`kbd_leave` on focus change). (2026-09-26)

- [x] **P1-T06-D Pointer enter/leave/motion + hit test.** Pointer `enter`/`leave`
  based on hit-testing surface rectangles against pointer position. Track
  pointer grab state. Maintain one increasing serial across all pointer/key
  events.
  **Acceptance:** `meson test -C build input-pointer` passes. (2026-09-26)

- [x] **P1-T06-E Input script.** Add `--input-script FILE` to main.cpp. Parse
  lines: `wait MS`, `key CODE press|release`, `motion X Y`, `button CODE
  press|release`, `focus lost|gained`, `quit`. Dispatch events through the
  input system during the event loop.
  **Acceptance:** `meson test -C build input-script` passes. (2026-09-26)

- [x] **P1-T06-F Test client `--report-input` + input-routing test.** Test client
  mode `--report-input` prints `kbd_enter`, `kbd_leave`, `key CODE STATE`,
  `ptr_enter`, `ptr_leave`, `motion X Y`, `button CODE STATE`. The
  `input-routing` test starts the compositor with a scripted input file, maps
  a client, and verifies event ordering.
  **Acceptance:** `meson test -C build input-routing` passes. (2026-09-26)

- [x] **P1-T07 Host window input replaces evdev.** In windowed mode select
  `KeyPress|KeyRelease|ButtonPress|ButtonRelease|PointerMotion|FocusChange|
  StructureNotify` on the X11 window, handle `WM_DELETE_WINDOW`, translate X
  keycodes (minus 8) and buttons (1→BTN_LEFT 0x110, 2→BTN_MIDDLE 0x112,
  3→BTN_RIGHT 0x111, 4/5→vertical axis) into the same internal event path used
  by `--input-script`. `ConfigureNotify` updates the viewport. Remove all
  `/dev/input` code. Windowed mode is exercised by a person; headless tests
  guard the shared path.
  **Acceptance:** `meson test -C build` still passes (6/6 OK);
  `grep -r "/dev/input" src` finds nothing; windowed mode — not observed
  (requires DISPLAY). (2026-09-26)

- [x] **P1-T08 Launcher: child lifecycle.** Already done in `src/launch.cpp`:
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
  headless, logs its connection and exit, and exits 0 itself. (2026-09-26)

- [x] **P1-T09 Lifecycle robustness.** (2026-09-26) `surface_destroy_callback` fires
  pending frame callbacks before moving the surface to the orphaned list. Two
  toplevels stack (later on top) via `wl_list_for_each_reverse`. Host window resize
  sends a new configure to the focused toplevel. A `lifecycle` test verifies three
  connect/disconnect cycles plus a fresh client that still receives `configure`.
  `meson test -C build lifecycle` passes; `meson test -C build-asan` passes all 8
  tests with no sanitizer errors.
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

- [x] **P1-T11 Project 1 wrap-up.** Reconcile `docs/STATUS.md`, finish
  `docs/handoffs/01.md` (what changed, exact commands, evidence, limitations,
  the first Project 2 task), and update `README.md`/`GETTING_STARTED.md` if
  commands changed.
  **Acceptance:** all Project 1 automated tasks ticked; `meson test -C build`
  passes; docs list P1-T10 as observed or not observed truthfully.

---

## Project 2: live application surface in 3D

Depends on Project 1 automated tasks (P1-T10 may still be unobserved).

- [x] **P2-T01 Math module.** `src/math.h` with `vec3`, `mat4`, `perspective`,
  `look_at`, `translate`, `rotate_y/x`, `multiply`, `transform_point`. Header
  only, no dependency. Unit test executable `tests/test_math.cpp`.
  **Acceptance:** `meson test -C build math` passes (projection of known points
  matches expected values within 1e-4).

- [x] **P2-T02 Perspective panel.** Add an MVP uniform to the shader. Introduce a
  `Camera` (position, yaw, pitch) and a `Panel` (position, size, normal).
  Draw the focused surface's texture on the panel; keep the 2D path behind
  `--flat`. Add `--camera X,Y,Z,YAW,PITCH` for tests. `tests/project_point.py`
  computes where a world point lands on screen with the same math.
  **Acceptance:** `meson test -C build render-panel` passes: with a fixed
  camera, the client colour is present at the projected panel centre and the
  clear colour at a pixel outside the projected quad.

- [x] **P2-T03 Live updates.** Re-upload textures only for surfaces committed
  since the last frame (track damage per surface). Frame callbacks keep
  flowing while the panel is shown.
  **Acceptance:** `meson test -C build render-update` passes: client commits
  red, screenshot A; commits blue, screenshot B; A is red and B is blue at the
  projected centre.

- [x] **P2-T04 Texture lifetime.** Textures are created on first commit,
  resized on buffer size change, deleted on surface destruction. Check
  `glGetError()` after each frame in debug builds and log once per error.
  **Acceptance:** `meson test -C build render-lifecycle` passes (three
  connect/draw/disconnect cycles, last screenshot valid, no GL error lines).
  (2026-09-27, fixed cyclic swizzle in `render_surface()`)

- [ ] **P2-T05 Accelerated client experiment (reopened 2026-09-29).** Add a
  test client mode `--egl` using `wayland-egl` that clears to a colour and
  swaps. Import its buffers with `EGL_WL_bind_wayland_display`
  (`eglBindWaylandDisplayWL`, `eglQueryWaylandBufferWL`, `eglCreateImageKHR`)
  or `zwp_linux_dmabuf_v1`. If the container has no usable GPU path, record
  the exact failure (extension list, error codes) in
  `docs/decisions/03-accelerated-buffers.md` and mark the task blocked, not
  done.
  **Acceptance:** a direct accelerated-client import regression passes with
  no client GPU-to-CPU readback/shm substitution; document buffer ownership and
  synchronization and run the full suite. The existing `render-egl` fallback
  test remains useful but cannot satisfy this acceptance by itself.
  **Current requirement:** direct import of an accelerated client's buffer must
  be demonstrated before ticking. P4-T13 owns the renewed investigation. The
  2026-09-27 PBuffer + `glReadPixels` + shm experiment remains historical
  fallback evidence; it does not demonstrate direct import. A negative research
  outcome is recorded as a feature blocker, not completion of this task.

- [x] **P2-T06 Camera movement.** WASD/arrow keys move, mouse look with the
  right button held (windowed mode). The same actions are available to
  `--input-script` through `key`/`motion`.
  **Acceptance:** `meson test -C build camera-move` passes: a scripted forward
  move changes the projected panel size between two screenshots.

- [x] **P2-T07 Frame timing.** `--stats` prints per-frame render time and bytes
  uploaded every 60 frames; record numbers for the shm path in the handoff.
  **Acceptance:** `meson test -C build` passes; `docs/handoffs/02.md` contains
  measured numbers with the machine description. (2026-09-27)

- [x] **P2-T08 Project 2 wrap-up and gate A record.** Handoff, status, decision
  on whether to continue with the current renderer/compositor integration.
  **Acceptance:** docs updated; every Project 2 task ticked or recorded as
  blocked with evidence. (2026-09-27)

---

## Project 3: world/application input state machine

- [x] **P3-T01 Explicit modes.** `enum class InputMode { World, Application }`
  with one owner. World mode: input drives the camera, clients get nothing.
  Application mode: input goes to the focused surface. `--input-script` gains
  `mode world|app`.
  **Acceptance:** `meson test -C build mode-routing` passes. (2026-09-27)

- [x] **P3-T02 Reserved shortcut and clean exit from application mode.**
  Configurable `--world-key` (default `F12`, evdev code 88) returns to world
  mode. Leaving application mode releases held keys (send `key` release for
  every pressed key), sends `modifiers`, keyboard `leave`, pointer `leave`.
  **Acceptance:** `meson test -C build mode-exit` passes (held key released,
  leave events observed). (2026-09-27)

- [x] **P3-T03 Host focus loss.** `focus lost` (X `FocusOut`) behaves like
  leaving application mode without changing the stored mode; `focus gained`
  re-enters the client only if the mode is still Application.
  **Acceptance:** `meson test -C build focus-loss` passes. (2026-09-27)

- [x] **P3-T04 Full-size presentation.** In application mode the focused
  surface is drawn 2D, scaled to fit the host window, and configured to the
  host size; returning to world mode restores the panel and the previous size.
  **Acceptance:** `meson test -C build present-fullsize` passes (client
  receives the new configure; screenshot is filled with the client colour). (2026-09-27)

- [x] **P3-T05 Targeting and selection.** Ray from the screen centre against
  the panel; `--input-script` `key 28 press` (Enter) on a targeted panel enters
  application mode.
  **Acceptance:** `meson test -C build select-panel` passes. (2026-09-27)

- [x] **P3-T06 Client exit during application mode.** Return to world mode,
  clear focus, no dangling pointers.
  **Acceptance:** `meson test -C build app-exit` passes under the ASan build.
  (2026-09-27) Fixed root cause: `destroy_listener.link` was shared between
  the Wayland resource's `destroy_signal` list and the seat's tracking list.
  `wl_list_insert` overwrote the link's prev/next, corrupting the destroy_signal
  iteration. Separated into `destroy_listener.link` (for destroy_signal) and
  `seat_link` (for seat tracking). Also added `seat_clear_focus_state()` helper
  and SIGSEGV handler for diagnostics.

- [x] **P3-T07 Project 3 wrap-up.** Handoff 03 created, status updated, host
  shortcuts documented. The world key (F12, evdev code 88) is intercepted by the
  compositor before reaching any Wayland client.

---

## Project 4: one-room end-to-end prototype

- [x] **P4-T01 Room geometry and collision.** Floor, four walls, a desk box,
  and a monitor frame as coloured, flat-shaded meshes; AABB collision keeps the
  camera inside the room and above the floor.
  **Acceptance:** `meson test -C build room-render` passes (floor colour at
  the bottom centre pixel, wall colour at the top centre pixel);
  `room-collision` passes (scripted walk into a wall stops at the bound).
  (2026-09-27)

- [x] **P4-T02 Monitor slot and teleport.** The launched client's panel sits in
  the monitor frame. Key `T` (evdev 20) teleports to a stored viewpoint facing
  the monitor (reduced-motion access).
  **Acceptance:** `meson test -C build teleport` passes.
  (2026-09-27)

- [x] **P4-T03 Milestone 1 scripted demonstration.** `tests/milestone1.sh`
  runs the nine concept steps headless: start, walk, launched client mapped
  on the monitor, approach, select, application mode, type (client reports the
  keys), return to world with the reserved key, client still connected and
  drawing.
  **Acceptance:** `meson test -C build milestone1` passes.
  (2026-09-27)

- [ ] **P4-T04 (human) Milestone 1 with a real terminal.** Same nine steps
  with `weston-terminal` in a host session. Record observations in
  `docs/STATUS.md`.

- [x] **P4-T05 Project 4 wrap-up and Decision gate A.** Handoff 04, status,
  and an explicit gate decision in `docs/decisions/`.
  **Acceptance:** `meson test -C build milestone1` passes (all 24 tests).
  (2026-09-27)

---

## Project 4 recovery: foundation stabilization

Goal: demonstrate a functioning nested graphical desktop and repair the
evidence/architecture gaps before feature expansion. These tasks do not
authorize a rewrite. If blocked, continue only independent eligible work.

- [ ] **P4-T06 Reconcile implementation and evidence.** Prerequisites: none.
  Inspect source, Meson registrations, tests, STATUS, decisions, and handoffs.
  Establish a baseline; identify partial registry code, stale next-task claims,
  contradictory swizzle explanations, orphaned-surface retention, and missing
  real-client evidence. Attribute old results; do not re-label them as new runs.
  **Acceptance:** reviewable evidence table and bounded follow-up list in
  `docs/handoffs/04-recovery.md`; run existing checks where available and record
  exact failures/missing prerequisites. No unchecked capability is called done.

- [ ] **P4-T07 Repair automation verification.** Prerequisites: P4-T06.
  Replace the plugin test's assumption that the live Projects 1–4 task list
  always contains unfinished work with stable fixtures. Add coverage for
  top-level task parsing and reopened gates; ensure Project 5 tasks are seen.
  Align generated run instructions with safe staging and prerequisite checks.
  Keep numeric IDs and explicit task scopes; fail closed for unrecognized scopes
  in normal runs. Do not bypass a gate because human tasks are excluded from
  the plugin's DONE check. This task authorizes a focused plugin/test change.
  **Acceptance:** `node --test .opencode/tests/*.test.js` passes; parsing tests
  show open P5 tasks, completed fixtures, blocked gate behavior, and unknown
  scope rejection. Existing interruption, limit, persistence and takeover
  behavior remains covered. Record a live run separately when available.

- [ ] **P4-T08 Audit resource and list lifetimes.** Prerequisites: P4-T06.
  Audit wl_surface, wl_buffer, xdg_surface, xdg_toplevel, listeners, registry
  links, and GPU cleanup. Test xdg_surface destruction before role assignment,
  legal role/surface teardown orders, abrupt disconnect during rendering,
  repeated connections, and concurrent clients. Handle protocol-invalid orders
  with specified errors rather than treating every order as valid. Bound test
  snapshots; remove unbounded retention of disconnected resources.
  **Acceptance:** add targeted lifecycle regressions, run the normal suite and
  `meson setup build-asan --buildtype=debug -Db_sanitize=address,undefined`
  (first time), `meson compile -C build-asan`, and
  `meson test -C build-asan --print-errorlogs`. No sanitizer errors, corrupted
  lists, double cleanup, or unbounded growth across repeated disconnects.

- [ ] **P4-T09 Correct pending/current surface state.** Prerequisites: P4-T08.
  Separate no-new-attach from attach-null. Apply attachment/damage/frame state
  on commit, preserve current content until then, and release each consumed
  buffer only once access has ended. Respect stride and protect shm access.
  Resource destruction invalidates pointers without inventing pixel ownership.
  **Acceptance:** new tests cover attach-without-commit, replaced pending attach,
  commit without attach, null attach/commit, repeated buffer reuse after release,
  resource destruction after release, and disconnect while rendering. Verify
  pixels/event order in normal and sanitizer builds against protocol semantics.

- [ ] **P4-T10 Verify formats and coordinates.** Prerequisites: P4-T09.
  Replace driver-specific color guesses with a tested format/stride/alpha path.
  Test unequal R/G/B values, XRGB and premultiplied ARGB, nontrivial stride,
  scaling, transforms, logical dimensions, letterboxing, and pointer mapping.
  Implement requirements for advertised versions; document unsupported protocols.
  **Acceptance:** format tests assert expected pixels in flat, panel, and
  application modes; coordinate tests hit corners/center after resize and scale
  changes. No globally selected --egl flag changes interpretation of ordinary
  clients' declared shm formats. All existing tests remain valid or their
  previous incorrect fixtures are corrected with a documented protocol reason.

- [ ] **P4-T11 Validate xdg-shell handshake and metadata.**
  Prerequisites: P4-T08, P4-T09.
  Check initial configure/ack/commit ordering, valid outstanding serials,
  unmap/remap, resize, title/app_id and window geometry. Audit advertised versions
  and mandatory requests; add minimal output/decoration support only where a
  chosen real client requires it. Do not claim broad toolkit support from this.
  **Acceptance:** protocol regressions cover valid repeated configure sequences
  and specified invalid cases without crashes, in normal and sanitizer builds.
  Metadata reaches runtime presentation; supported-version claims are accurate.

- [ ] **P4-T12 Native nested Wayland host experiment.** Prerequisites: P4-T06.
  Investigate a host Wayland window with EGL, input/focus, resize, and close.
  State at the 2026-09-29 merge: commits d4f8551 through a615f18 already removed
  the X11 host window and present through a `wl_egl_window` client connection
  (`src/display.cpp`), with seat/relative-pointer listeners; the same change
  reintroduced direct `/dev/input` evdev reading in windowed mode
  (`input_init()` in `src/input.cpp`). Handoff 04-host-window reports severe
  flicker and no host input; nothing about this path is verified. Preserve the
  headless path. Keep the host connection distinct from Mansion's private client
  socket and launcher environment. Diagnose the flicker and missing host events
  with logs (`WAYLAND_DEBUG=1`) rather than adding further workarounds, and
  decide whether host input must come from the host seat only.
  **Acceptance:** a bounded prototype plus reproducible commands/logs demonstrates
  rendering and host events, or a precise blocker/capability report explains why
  it cannot. Record required versions and what remains unobserved; a report alone
  does not mark a native Wayland backend implemented. P4-T16 needs a usable host.

- [ ] **P4-T13 Accelerated buffer import investigation.** Prerequisites: P4-T09.
  Recheck EGL client/display extension strings and extension entry points with
  eglGetProcAddress. Compare EGL Wayland binding and linux-dmabuf import,
  including advertised protocol, formats/modifiers, ownership and synchronization.
  Record Mesa/driver/GPU, errors and copy points; keep wl_shm functioning.
  **Acceptance:** a real accelerated Wayland client submits a buffer that Mansion
  directly imports and displays, or Decision 03 documents an exact reproducible
  blocker and the changes needed. Report research completion separately from
  P2-T05 feature completion. PBuffer/readback/shm remains a fallback experiment.

- [ ] **P4-T14 Bounded wlroots suitability experiment.**
  Prerequisites: P4-T06; use P4-T12/P4-T13 findings when available.
  In an isolated experiment, evaluate nested/headless backends, buffer import,
  surface trees and seat input while retaining custom C++ spatial rendering.
  Compare integration burden with the implemented libwayland-server path.
  Pin the evaluated version; record dependency proposals before adding them.
  **Acceptance:** a small demonstrator or exact dependency/environment blocker,
  and an ADR comparing measured integration work, ownership, risk, and next steps.
  No production stack replacement or bulk rewrite is part of this task.

- [ ] **P4-T15 Introduce one architecture boundary.**
  Prerequisites: P4-T08, P4-T10, P4-T11, P4-T14.
  Map current code to ARCHITECTURE.md. Extract the smallest useful runtime
  presentation/world boundary from display.cpp, documenting ownership, logical
  dimensions, damage, invalidation and input coordinates. Use existing math.
  **Acceptance:** existing demonstrations/tests pass, plus one lifetime regression
  through the new boundary. World data has no persistent Wayland/GL/PID identity.
  Document actual changes and deferred boundaries; no new general engine/ECS.

- [ ] **P4-T16 Prepare a reproducible real-terminal trial.**
  Prerequisites: P4-T10, P4-T11, P4-T12, P4-T15.
  Launch an installed native Wayland terminal via Mansion's launcher on a usable
  host backend; log its connection/mapping and updates. Fix the smallest blocking
  protocol/host issue; record missing packages for a person to install. Publish
  exact flat and room commands and diagnostics in ACCEPTANCE.md. Log assertions
  supplement the visual trial; they do not establish usability.
  **Acceptance:** reproducible launch evidence from a real terminal and a complete
  manual procedure. If the terminal or host window is unavailable, leave this
  task blocked; do not replace it with the synthetic test client.

- [ ] **P4-T17 (human) Foundation desktop demonstration.**
  Prerequisites: P4-T16. Follow ACCEPTANCE.md in flat and room modes: readable
  text, typing, pointer selection, scrolling, resize, repeated enter/return,
  host focus loss, terminal close/reopen, and safe Mansion exit. Record actual
  screenshots, application/backend versions, observations and limitations.
  **Acceptance:** a person's recorded successful checks with date and source
  revision. The same evidence may also satisfy P1-T10 and P4-T04 if it covers
  their full procedures. Never tick those automatically from synthetic tests.

- [ ] **P4-T18 Foundation decision gate.**
  Prerequisites: P4-T06 through P4-T17, including human evidence.
  Review failures, sanitizers, real terminal behavior, backend/stack experiments,
  and architecture boundaries. Record accepted/deferred/blocked in Decision 05.
  A bounded shm-only next phase may be accepted explicitly for the tested client
  and hardware; GPU-import capability remains blocked and P2-T05 stays open.
  A required stack migration needs its own bounded plan before proceeding.
  **Acceptance:** no required foundation check is missing or failing; ADR names
  the accepted backend/client/configuration and limits. Only an accepted gate
  enables P4-T20. Research reports alone cannot satisfy the real-terminal gate.

---

## Project 4 visual slice: the first presentable room

Goal: one coherent textured study with a live application, ordinary focus mode,
and a working door. This is an integrated desktop milestone, not a detached
scene viewer. Detailed PBR, shadows and creature animation remain later work.

- [ ] **P4-T20 Minimal scene entities and transforms.** Prerequisites: P4-T18.
  Introduce data-driven entity/parent IDs and mesh/material instances using the
  established architecture boundary. Keep runtime application bindings separate.
  **Acceptance:** transform/hierarchy and invalid-parent tests plus the existing
  room/terminal checks pass. Parent movement changes child world transforms.

- [ ] **P4-T21 glTF/GLB asset loading.** Prerequisites: P4-T20.
  Choose a small loader with a dependency ADR; document the supported static mesh,
  node, UV/index/normal and texture subset. Import one Blender-exported fixture;
  handle missing/malformed/unsupported assets without corrupting the desktop.
  **Acceptance:** load/render the fixture with tested transforms and index types;
  invalid asset tests fail with useful diagnostics; provenance/license recorded.

- [ ] **P4-T22 Textures and basic materials.** Prerequisites: P4-T21.
  Implement UVs, base color textures/factors, sampler behavior and a documented
  color-space/alpha policy. Separate scene material sampling from client pixels.
  **Acceptance:** asymmetric texture/UV fixtures match expected locations/colors,
  missing textures have a clear fallback, and real app text colors are preserved.

- [ ] **P4-T23 Ambient and directional lighting.** Prerequisites: P4-T22.
  Use normals and simple predictable lighting; retain an unlit application path.
  Measure GLES2 compatibility. Consider GLES3 only through a bounded ADR-backed
  capability experiment, retaining the baseline if unsupported.
  **Acceptance:** controlled normal/light fixtures and application pixel checks
  pass. Record frame times on the named hardware; no required PBR/shadow engine.

- [ ] **P4-T24 Furnish one coherent study.** Prerequisites: P4-T23.
  Assemble textured walls/floor, desk, shelf, monitor and door from licensed assets.
  Document scale, collision bounds and placement slots; avoid decorative geometry
  that obstructs navigation or app readability. Use a modest reproducible workload.
  **Acceptance:** scene reload is deterministic; camera collision/teleport tests
  pass; record screenshots for later human assessment, asset license and budgets.

- [ ] **P4-T25 Object targeting and placement.** Prerequisites: P4-T24.
  Ray-pick supported furniture/objects, show selection feedback, and move one
  object between valid slots. Keep world actions out of application input.
  **Acceptance:** deterministic ray/occlusion and valid/invalid slot tests pass;
  the artifact/entity keeps its ID while moved. Restart persistence waits for P6.

- [ ] **P4-T26 Interactive door.** Prerequisites: P4-T24, P4-T25.
  Add a door hinge transform and bounded opening/closing animation, updated
  collision, and a reduced-motion alternative. A door need not lead to a second
  room yet; maintain the player's valid position when closing it.
  **Acceptance:** scripted targeting/toggle updates door transform and collision;
  repeated toggles and occupied doorway cases do not trap the camera.

- [ ] **P4-T27 (human) Live terminal in the furnished study.**
  Prerequisites: P4-T24 through P4-T26.
  Bind the real terminal to the scene monitor. Follow ACCEPTANCE.md for walking/
  teleport, selection, readable full-size typing/click/scroll, return to world,
  object move, door interaction and close/reopen. Assess comfort and appearance.
  **Acceptance:** a person's recorded results and screenshots; the same live
  client survives mode transitions and furniture does not hide important UI.

- [ ] **P4-T28 Presentable-room gate and handoff.**
  Prerequisites: P4-T20 through P4-T27 and continued P4-T18 acceptance.
  **Acceptance:** relevant automated checks and human furnished-room criteria
  pass; document exact scene/client/backend, frame-time/copy measurements and
  limitations in handoff 04-visual. Record the gate decision before resuming P5.

---

## Projects 5–8: useful persistent workspace

Each project starts with an expansion task. Autonomous sessions perform the
expansion task, then work the resulting list.

- [x] **P5-T00 Expand Project 5.** Existing expansion revised below; feature
  tasks require P4-T18 and P4-T28 accepted. Historical completion: 2026-09-28.

  Historical 2026-09-28 expansion (superseded, kept as evidence): the earlier
  nested sub-tasks "P5-T01 Window registry" and "P5-T02 Alt+Tab focus cycling"
  were implemented in bb8cd9d (`toplevel_list`/`toplevel_count` in
  `MansionCompositor`, `xdg_shell_cycle_focus()`, `--tab-key`, input-script
  `tab`/`shift_tab`) and are covered only by the synthetic `focus-cycle` test;
  the `toplevel-registry` test named in that old acceptance was never added.
  This partial code is the "partial registry code" the revised P5-T01 must
  inspect. It does not tick the revised P5-T01/P5-T02 below, whose acceptance
  (same-client windows, unmapped windows, teardown orders, sanitizers, held-input
  release) has not been demonstrated.

- [ ] **P5-T01 Window registry and surface trees.** Prerequisites: P4-T28.
  Inspect partial registry code before adding another registry. Track runtime
  toplevel handles, mapped state, metadata and related surfaces across clients.
  Registration/unregistration is idempotent for role teardown and disconnect;
  invalidate every focus/render reference safely. No persistent resource pointers.
  **Acceptance:** new registry tests cover multiple clients, multiple windows
  from one client, unmapped windows, legal teardown, and abrupt disconnect.
  Counts and lifetime assertions pass under sanitizers and the normal suite.

- [ ] **P5-T02 Focus cycling and input policy.** Prerequisites: P5-T01.
  Define keyboard-only world-mode next/previous-window actions and one reserved
  application-mode escape. Make defaults and modifier requirements unambiguous;
  ordinary application Tab and typing reach the app. World mode does not deliver
  client keyboard input just because a candidate is selected. Release held
  keys/buttons on activation/focus loss and restore surface-local pointer mapping.
  **Acceptance:** two clients and two same-client windows cycle correctly; world
  selection emits no app typing, activation delivers input to only the chosen
  window, focus loss/exit releases held input, and destroyed candidates are skipped.

- [ ] **P5-T03 Positioners and xdg_popup.** Prerequisites: P5-T01, P5-T02.
  Implement required positioner state, validation, parent-relative placement,
  configure/ack/commit ordering, popup stacking, hit testing, and dismissal.
  Validate grab seat/serial/topmost rules; do not invent a synchronous protocol
  handshake or grab only the parent's rectangle. Compose popups with their parent
  in application mode and world preview. Advertise only implemented versions.
  **Acceptance:** tests exercise configure and actual rendered popup pixels/input,
  constrained placement, nested popups, dismissal, invalid grab/parent sequences,
  and parent disconnect. A real client's menu is tested separately in P5-T10.

- [ ] **P5-T04 Transient/dialog associations.** Prerequisites: P5-T01.
  Handle set_parent with protocol validation, cycles, changing parents, and
  destruction. Define activation/grouping policy without hiding necessary dialogs
  from the user. App IDs and PIDs may assist grouping but are not window identity.
  **Acceptance:** tests cover same-process multiple windows, parent removal,
  dialog focus/visibility and invalid parent cases, with no dangling references.

- [ ] **P5-T05 Request focused-window close.** Prerequisites: P5-T01, P5-T02.
  A documented compositor close shortcut sends xdg_toplevel.close to the target.
  Do not destroy its xdg_surface/wl_surface/buffer, kill the client process, or
  assume disconnect. The client may prompt, defer, or decline. Shortcut handling
  must not confuse closing a window with shutting down launcher-owned children.
  **Acceptance:** accepting client closes cleanly; declining/deferred client stays
  mapped and interactive; two windows in one process show only the selected
  window requested to close. Unrelated clients survive. Test under sanitizers.

- [ ] **P5-T06 Window list and manual assignment.**
  Prerequisites: P5-T01, P5-T02, P5-T04.
  Show a searchable/selectable world overlay with title/app ID and active state.
  Support explicitly assigning an ambiguous live window to an artifact/monitor.
  Keep overlay UI outside application pixels, preserve accessible keyboard use,
  and avoid duplicating a fake application list inside the surface texture.
  **Acceptance:** automated selection/assignment and teardown tests plus a manual
  overlay readability trial; title/app ID changes appear without misbinding.

- [ ] **P5-T07 Subsurface composition.** Prerequisites: P5-T01, P5-T03.
  Implement or integrate the surface-tree behavior required by the chosen clients:
  parent-relative position, stacking, synchronized/desynchronized commits, input
  regions and lifecycle. Split into bounded tasks before implementation as needed.
  **Acceptance:** test synchronized/desynchronized updates, overlapping content,
  correct input targets and parent destruction; real-client coverage in P5-T10.

- [ ] **P5-T08 Real multi-application integration harness.**
  Prerequisites: P5-T03 through P5-T07.
  Prepare native terminal/browser/editor launch commands with isolated test data
  and backend flags where needed. Report unsupported protocols and accelerated
  paths precisely. Do not fall back silently to the host X server or claim that
  startup logs prove visible usability.
  **Acceptance:** reproducible launch/mapping logs and ACCEPTANCE.md procedures
  for three named app versions, menus/dialogs and independent close. Missing
  applications/backend capabilities remain blockers.

- [ ] **P5-T10 (human) Multiple applications acceptance.**
  Prerequisites: P5-T08. Verify terminal, browser and editor coexist, focus/input
  remain correct, menus/dialogs appear at the correct parent, declined close is
  honored, and closing one window leaves the others working.
  **Acceptance:** dated observations, screenshots, exact versions and limitations
  in ACCEPTANCE/STATUS. This evidence must exist before P5-T09 wrap-up.

- [ ] **P5-T09 Project 5 product gate and wrap-up.**
  Prerequisites: P5-T01 through P5-T08 and P5-T10.
  **Acceptance:** normal/sanitizer suites and human multi-application criteria
  pass; record tested scope, surface-tree ownership and remaining compatibility
  limits in handoff 05. A task marked blocked/not observed is not a passing gate.

- [ ] **P6-T00 Expand durable placement tasks.** Prerequisites: P5-T09.
  Plan a versioned SQLite schema and migrations, resource/artifact/world-entity
  separation, stable UUIDs, room/parent-relative transforms, slot-based movement,
  transactions, export/reset and interrupted-write recovery. Runtime handles,
  Wayland resources, PIDs and GPU IDs are excluded from persisted identity.
  Closing an app preserves its inactive artifact; moving a file artifact does
  not implicitly move the underlying file. Add tests for parent movement,
  deleted resources, stale runtime bindings, duplicate names, restarts and failed
  writes. Record SQLite dependency/version choice before adding it.
  **Acceptance:** bounded dependency-ordered tasks and migration/identity ADR;
  each task has commands, failure cases and a product restart demonstration.

- [ ] **P7-T00 Expand launch and restoration tasks.** Prerequisites: Project 6
  product gate (to be added by P6-T00).
  Plan argv-safe launch recipes, explicit restore choices, reused-process and
  multiple-window ambiguity, manual assignment, failed launches, duplicate
  suppression, and shutdown ownership. Restoration reopens resources; it does
  not recover arbitrary unsaved application memory.
  **Acceptance:** bounded tests and a manual launch/restart sequence, including
  ambiguous window binding and failure without duplicate artifacts.

- [ ] **P8-T00 Expand useful-workspace tasks.** Prerequisites: Project 7 gate
  (to be added by P7-T00).
  Plan search, inactive artifacts, keyboard selection, list/teleport access,
  reduced-motion options and a real usability trial. Searching must not populate
  the room with every indexed file. Path illumination is the later 8N track.
  **Acceptance:** bounded tasks and a demonstration finding/reopening a placed
  item after restart, with ordinary files remaining accessible from the host.

Projects 9 and later, including the 8N/14R/15I spatial feature tracks, are
defined in the roadmap. Expand them only after their stated prerequisites.
