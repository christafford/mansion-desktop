# Task list

This is the executable form of [PROJECT-ROADMAP.md](../PROJECT-ROADMAP.md).
Autonomous sessions work from this file: take the first eligible unchecked
task in scope, prove its acceptance, record the evidence, and commit.

## Current execution order (2026-10-02 audit, Decision 06)

1. Project 21 below: Godot/Poly Haven study and independent compositor bridge.
2. Its furnished live-terminal gate, then the useful two-room workspace.
3. Final personal review, then separately scoped future roadmap programs.

Decision 06 supersedes P4-T18/P4-T28 as prerequisites for this work. Historical
checkmarks do not prove the new bridge works; incomplete old tasks remain open.
Legacy Projects 5–8 are source requirements/reference work, not a competing
autonomous scope while Project 21 is active. New integration gate P21-T19 permits
the dependent useful-workspace features; P21-T33 prepares final owner review.

## Project 21: Godot and Poly Haven workspace

Current priority, authorized by [Decision 06](decisions/06-godot-poly-haven.md).
Implement these tasks; another planning-only pass does not satisfy them. Visual
tasks T01–T09 and core tasks T10–T18 progress independently from T00. Choose the
first eligible unchecked task; skip a recorded blocker to another eligible task
in this scope. Human review is at the end. Prior P4-T12–T28 tasks are deferred
or superseded as an execution schedule; they remain honest unchecked history.
P2-T05 GPU import remains open and is not a prerequisite for this CPU bridge.

## Instructions for the next autonomous run

T36–T44 passed; current recovery continues with T45; do not start a broad
autocontinue run.
The eventual first useful product milestone ends at T19.
Do not use a broad Projects 1–21 scope: that mixes deferred legacy tasks with
this work. This document update does not itself start an autonomous run.

1. Read STATUS.md, then the first eligible task below. Follow the recovery checkpoint in STATUS.md.
   T00, T01, T02, T03 and T10 were **reopened** on 2026-10-02. Their previous
   checkmarks did not meet acceptance; do not restore them from an old handoff.
2. Execute the numbered steps in that task. Reuse the existing code and caches.
   Repair implementation; a new plan, empty API, successful compile, downloaded
   file or screenshot of a primitive is not the requested feature.
3. Run every listed pass check. Save commands, exit codes, logs and required
   rendered captures. Read error logs even when the process exits 0. Inspect
   actual images for visual tasks. Headless mode does not render acceptance
   pictures; use a working graphical renderer for captures.
4. Tick only after all checks pass. Update STATUS.md and the current handoff,
   then commit only that task's files. Do not claim a feature works because an
   unrelated C++ test suite passes. Do not mark human tasks complete.
5. If unfinished, leave it unchecked and write `Next: P21-Txx`, the failing
   command, actual error and exact next code change in STATUS.md. Continue the
   same task next turn. A task may take many turns; do not skip it for difficulty.
6. A blocker needs a fresh failed command and a reason no safe local fix exists.
   Test installed tools before saying they are missing. Do not wait for a GUI
   settings page to register a GDExtension. If blocked, take another eligible
   task in scope: T10 needs only T00; T11 also needs T01. If no task is eligible,
   record all blockers and end with `AUTOCONTINUE_BLOCKED`.
7. End with `AUTOCONTINUE_DONE` only after every non-human task in the configured
   scope passes. The current recommended scope is the single recovery task in
   STATUS.md. The historical T00–T19 range is not active. Otherwise end with
   `Next: <task id>`.

**Original full milestone sequence (resume after recovery):** T00 → T01 → T02 → T03 → T04 → T05 → T06 → T07 → T08 →
T09 → T10 → T11 → T12 → T13 → T14 → T15 → T16 → T17 → T18 → T19.
The explicit Depends lines control eligibility; the sequence prioritizes a
visible real-asset room while allowing independent core work if art is blocked.
T20–T35 retain the later roadmap; do not start them in the recommended scope.

The plugin recognizes only column-zero `- [ ] **P21-T00 ...` task lines and
numeric IDs. It does **not** enforce Depends or validate evidence. Preserve all
IDs and dependency edges. Do not hide subtasks in nested checkboxes or use ID
suffixes. Split only when a task has separate acceptance boundaries; add unused
numeric IDs and include them in the authorized scope before relying on them.

### Bounded recovery authorized on 2026-10-03

Run these small recovery tasks before resuming the original milestone sequence.
They do not silently check off broader art, toolchain or live-terminal tasks.
Do not launch another broad overnight range before the new boundaries pass.

- [x] **P21-T36 Recover a visible Godot frontend using existing assets.**
  **Depends:** none (bounded recovery using the installed tool/cache).
  Preserve existing work; isolate the crashing addon; stage complete glTF sources;
  render real textured furniture with light; implement collision-based walking,
  mouse look/release and focus cleanup; repair the validator's failure detection.
  **Acceptance:** inspected GPU-rendered views, graphical controller/collision
  smoke, successful import/runtime validation, and rejected invalid-script fixture.
  Evidence: [recovery handoff](handoffs/14-godot-frontend-recovery.md).
  This is a frontend preview, not finished art or a live desktop.

- [x] **P21-T37 Prove a minimal standard godot-cpp binding.**
  **Depends:** P21-T36.
  Use the installed compatible godot-cpp and official initialization pattern.
  Build one isolated extension class with one callable method. Run repeated
  instantiate/call/free and clean process shutdown. Keep the old crashing addon
  isolated; no Wayland, pixel pointers or legacy renderer in this probe.
  **Acceptance:** actual Godot script assertions pass through the C++ class;
  record exact engine/bindings/build identities and commands. This is a binding
  prerequisite only, not completion of the compositor adapter in T11.

  Evidence: [binding recovery](handoffs/16-godot-binding-recovery.md).

- [x] **P21-T38 Connect the recovered binding to a verified compositor lifecycle.**
  **Depends:** P21-T37.
  This is bounded recovery of T10/T11, not acceptance of either full gate.
  Inspect the partial core and preserve the existing compositor's real protocol
  behavior. Reuse/extract at one boundary; no replacement compositor or legacy
  GL renderer inside the new extension. Give one native object explicit
  start/pump/stop ownership. Use `wl_event_loop_dispatch(server_loop, 0)` and
  flush server clients; never pass a server display to a Wayland client API.
  Test an actual fixture connecting to the returned private socket, discovery,
  repeated start/stop, disconnect, and socket removal. Run the standalone core
  test and Godot lifecycle test plus C++ regressions. No snapshots or input yet.
  **Acceptance:** a real client connects and is served while Godot continues
  processing frames, repeated shutdown is clean, and no GL/display dependency
  enters the reusable core. Preserve pending provenance/art tasks separately.

  Evidence: [compositor lifecycle](handoffs/17-godot-compositor-lifecycle.md).

- [x] **P21-T39 Restore xdg-shell and seat in the recovered core runtime.**
  **Depends:** P21-T38.
  Inspect and adapt the existing `xdg-shell.cpp` and `input.cpp`; keep policy and
  legacy renderer calls outside the reusable core. Register the real globals
  through the owned runtime; preserve existing protocol behavior and tests.
  Extend the actual client fixture to verify initial xdg configure/ack/commit,
  seat discovery and client/resource teardown through Godot. Keep the headless
  core library independent of EGL/GLES and the legacy display loop.
  **Acceptance:** fixture handshake and repeated lifecycle checks pass through
  both standalone core and Godot, regression suite passes, and ownership is
  explicit. Terminal usability, input mapping and live pixel delivery remain
  separate gates. Do not substitute stub globals or a new compositor.

  **Evidence (2026-10-03):** 16 standalone + 16 Godot real-client trials,
  32/32 Meson tests and standalone ASan/UBSan with leak detection; see
  [handoff 18](handoffs/18-godot-window-protocol.md).

- [x] **P21-T40 Deliver owned shm frames through the recovered binding.**
  **Depends:** P21-T39.
  Work in `compositor-core.cpp`, `compositor-runtime.*` and the standard
  `compositor_session.*` binding. Preserve the existing legacy renderer.
  First fix pending/current buffer semantics, including explicit null attach,
  buffer replacement/destruction and detach. Copy supported ARGB/XRGB shm pixels
  under Wayland read access into bounded, owned CPU storage with explicit size,
  stride/format and revision. Release client buffers only after the copy; finish
  frame callbacks after the runtime has consumed the commit. Expose runtime-only
  surface handles and copied frame data to Godot, never raw pointers or durable
  identities derived from Wayland IDs. Reject unsupported formats explicitly.
  Extend the fixture to send changing asymmetric pixels with row padding and to
  replace/destroy/detach buffers. Test an old snapshot after replacement and
  disconnect; ensure handles from a previous session cannot alias a new window.
  **Acceptance:** exact dimensions, orientation, channel order and byte values
  verified through standalone runtime and Godot; release/frame callbacks and
  teardown asserted; sanitizer and regression checks pass. This is frame
  transport acceptance, not a rendered monitor or terminal usability claim.
  On completion, add the next bounded task for ImageTexture presentation on the
  study monitor, with actual rendered evidence and real terminal launch.

  **Evidence (2026-10-03):** real shm bytes verified at 32 barriers in standalone
  and 32 in Godot, six negative cases in each, 33 Meson tests and standalone
  sanitizer checks; see [handoff 19](handoffs/19-godot-owned-frames.md).

- [x] **P21-T41 Present live terminal output on the Godot study monitor.**
  **Depends:** P21-T40.
  Build/install the recovered standard binding for the `world/` development
  project; keep the old crashing addon ignored. Reuse the owned session and
  pump it from the scene's frame loop. Select a real xdg toplevel, not cursor or
  other role-less surfaces; expose minimal window metadata if needed. Update an
  unlit ImageTexture only on new frame revisions, replacing it when dimensions
  change. Apply inverse buffer transform/scale consistently; preserve the room,
  controller and real assets. Clear live content on detach/destruction.
  Inspect installed terminals and launch one native software-rendered client on
  the returned private socket with isolated process environment. Track only
  owned children and clean them up with the session. Fix required protocol gaps
  at their existing boundary; do not replace missing terminal output with a mock.
  **Acceptance:** documented launch command produces real, changing terminal
  text inside the study monitor. Inspect GPU captures of the room and close-up;
  verify orientation, color, aspect ratio, update and disconnect behavior, and
  rerun scene/controller, bridge and C++ regressions. Label observation as agent
  evidence. This gate covers live output; follow with a bounded task for ordinary
  typing/pointer input, focus/application mode and resize/close acceptance.

  **Evidence (2026-10-03):** real Weston 15.0.1 terminal, inspected GPU room and
  close-up captures with changing output, disconnect/owned shutdown cleanup,
  eight inverse transforms, eleven GPU color patches, controller/bridge checks
  and 33 Meson tests pass. See [handoff 20](handoffs/20-godot-live-terminal.md).

- [x] **P21-T42 Type into the real terminal in Godot application mode.**
  **Depends:** P21-T41.
  Read handoff 20 and inspect `seat.cpp`, `compositor-runtime.*`,
  `compositor_session.*`, `terminal_screen.gd` and `game_world.gd` before editing.
  Keep the current scene and live output working. Do not revive the ignored addon.

  1. Expose keyboard focus and key delivery through the owned runtime/session,
     reusing the independent seat. Resolve only valid runtime toplevel handles;
     never pass resource addresses or use the legacy global seat. Test focus
     enter/leave and exact key/modifier events through a real protocol fixture.
  2. Add explicit world/application mode. In world mode Enter activates the
     current live terminal. Present its same live texture in a readable flat
     view with preserved aspect ratio and unmodified client colors. Release
     captured mouse/movement on entry and suspend camera input while active.
  3. Map Godot physical keys to Linux evdev/XKB input explicitly. Support ordinary
     letters/digits, punctuation, Shift/Ctrl/Alt, arrows, Tab, Backspace, Enter
     and Escape with press/release and repeat behavior. Do not forward Godot
     numeric key values as Linux keycodes. State the tested keyboard layout.
  4. Reserve Ctrl+Alt+Escape for return to world mode; document it in the UI.
     Ordinary Escape, WASD, M, Home and Tab must reach the application. Clear
     held keys/modifiers on mode exit, host focus loss and window destruction.
     Do not capture keys outside the focused Godot host window.
  5. Run a real `/bin/sh` inside Weston terminal. Through the actual input route,
     type a command that produces distinctive output, edit it, exercise modifiers
     and repetition, return to world and reactivate the same live window. Capture
     the result; test disconnect/focus loss with a held key and no stuck movement.

  **Acceptance:** protocol input assertions, actual shell-command output and
  rendered application view pass; world/controller, screen-color, binding and
  full C++ regressions pass. Label injected events and agent observation honestly;
  no human gate is ticked. Pointer/resize/client-close remain T43–T45. Update status
  and the next handoff, then commit only task files.

  **Evidence (2026-10-03):** two-client keyboard protocol and sanitizer checks,
  27 key-mapping assertions, real shell command/edit/repeat/interrupt tests,
  native-size application capture with 7,571 pixel comparisons, mode/focus-loss/
  shell-exit checks, scene/binding regressions and 33 Meson tests pass. Input
  events are agent-injected, not a physical/human trial. See
  [handoff 21](handoffs/21-godot-terminal-keyboard.md).

- [x] **P21-T43 Route terminal pointer and scroll in application mode.**
  **Depends:** P21-T42.
  Read handoff 21. Inspect the existing seat/pointer resource implementation and
  legacy input policy; Godot must use its owned seat, never the legacy global.
  Expose surface-local pointer enter/motion/button/scroll through the runtime and
  standard binding. Map from `application_mode.gd`'s actual TextureRect geometry
  to logical surface coordinates, including buffer scale/transform. Do not send
  letterbox-margin or world-mode clicks to clients. Keep pressed-button/grab state
  bounded to a live surface and release it on mode exit, host focus loss and
  disconnect. Preserve the keyboard path and the world controller.
  **Acceptance:** two-client protocol tests assert focus isolation, coordinates,
  button mapping, scroll and held-button teardown; mapping tests cover native and
  shrunk views, corners, margins and scale/rotation. In real Weston terminal,
  select text by dragging, exercise scrollback and return to world with no stuck
  button/camera motion. Capture agent-observed evidence; run scene/bridge/C++ and
  relevant sanitizer checks. Client cursor/selection-protocol limitations must
  be recorded honestly. Resize and explicit shell close/relaunch follow below.
  **Verified 2026-10-03:** native isolation/grab/lifetime tests and sanitizers,
  302 mapping assertions, real Weston selection/scroll, inspected captures,
  scene/bridge regressions and 33/33 Meson tests. See
  [handoff 22](handoffs/22-godot-terminal-pointer.md). Cursor images, clipboard and
  popup menus remain unsupported; a popup request can disconnect a client.
  This checkmark is not the broader terminal usability gate.

- [x] **P21-T44 Resize the terminal through xdg configure.**
  **Depends:** P21-T43.
  Read handoff 22. Inspect `xdg-shell.cpp` and existing configure/ack/commit tests
  before adding a runtime resize request. Request a bounded application size when the
  Godot viewport changes; respect window geometry/client decorations, minimum
  sizes and the client's committed choice. Keep old pixels valid until a new
  buffer is committed. Do not claim that scaling ImageTexture resizes the client.
  Recompute the flat-view placement and pointer transform from committed state.
  **Acceptance:** fixture tests cover configure serials, acknowledgements,
  delayed/declined size changes, repeated resize and destruction while pending.
  Resize the real terminal twice in both directions; text/rows change without
  distortion, typing and pointer mapping remain correct, and the world preview
  updates. Inspect GPU captures and run scene/bridge/C++ regressions.
  **Verified 2026-10-04:** configure/ack/commit and constraint/geometry fixture
  tests, resize sanitizers, four real Weston resize/typing/selection trials,
  8,573 native-size GPU pixel samples, inspected captures, full scene/bridge
  checks and 33/33 Meson tests pass. See [handoff 23](handoffs/23-godot-terminal-resize.md).
  Physical window-manager drag-resize and human usability are not inferred.

- [ ] **P21-T45 Request client close and explicitly relaunch the terminal.**
  **Depends:** P21-T44, P21-T48.
  Read handoffs 23 and 25. T48 now implements the wheel, live-window selection
  and terminal relaunch; reuse those paths and tests. Add a documented application close action using
  `xdg_toplevel.close`. Never destroy the client's resources or kill its process to simulate acceptance.
  Keep this separate from launcher-owned process shutdown. Add an explicit world
  action to launch the terminal after exit; reject duplicate launch while the
  owned client is still running. Clear stale focus/textures/handles and assign
  the new live window without treating PID or Wayland IDs as persistent identity.
  **Acceptance:** accepting/deferred/declining fixture clients and two windows in
  one process prove target-specific close; real shell/UI close, relaunch and
  repeated world/application transitions work with keyboard, pointer and resize.
  Inspect captures, verify owned child/socket cleanup, and run regressions plus
  lifecycle sanitizers. Add bounded follow-up tasks for cursor/clipboard/popup
  limitations before claiming full usability, and an integration/owner-trial task.
  Broader human and Project 21 product gates stay open until their evidence exists.


- [x] **P21-T46 Adjust world mouse navigation at the owner's request.**
  **Depends:** P21-T36. Separately authorized on 2026-10-04, before continuing T45.
  Reverse horizontal mouse look. On right-button release, ease pitch back to the
  arrival angle over 0.2 seconds; retain yaw and held walking keys. Re-grabbing
  must interrupt the return. Home, focus cleanup and application mode retain
  control of the camera/input boundary.
  **Acceptance:** graphical controller assertions for both horizontal directions,
  upward/downward return, yaw preservation, held keys, interrupted return and
  Home; import/script validation and existing study/terminal regressions pass.
  See [handoff 24](handoffs/24-godot-navigation.md). Human comfort is not inferred.

- [x] **P21-T47 Turn with A/D; strafe while the right mouse button is held.**
  **Depends:** P21-T46. Separately authorized by the owner on 2026-10-04 and
  revised the same day: A turns left and D turns right at a fixed rate by
  default; while right mouse is held they strafe instead. Focus loss and Escape
  clear the look state with the held keys.
  **Acceptance:** graphical controller assertions that A/D turn without moving
  by default and strafe without turning while held; import/script validation
  passes. Verified 2026-10-04: controller test 56 textured surfaces, zero
  failures in two consecutive runs; the first version's assertions failed when
  the turn rate was zeroed.

- [x] **P21-T48 Add the recent-application wheel and desktop application search.**
  **Depends:** P21-T44, P21-T47. Owner-authorized launcher work precedes T45;
  [Decision 07](decisions/07-application-launcher.md) records this scope and dependencies.
  Use the requested Advanced Radial Menu asset, eight unique recent desktop IDs
  clockwise from twelve o'clock, and center-click searchable application names
  with installed icons. Persist history; do not invent populated recent slots.
  Discover freedesktop applications without a KDE service dependency. Launch on
  the private Wayland display, report unsupported/failed launches, preserve
  application input and own child cleanup. Support Terminal=true entries through
  Weston; permit selecting an existing recent window and relaunch after exit.
  **Acceptance:** desktop metadata/Exec/environment tests, graphical center-click,
  typed search, eight-slot ordering/history/cancel checks; a real installed app
  launches, receives input, returns to the world, and the terminal can relaunch.
  Inspect labeled GPU captures and run the study/terminal regression suite.
  Full graphical-app/protocol compatibility and T45 close requests stay separate.
  **Verified 2026-10-04:** six Python tests, graphical launcher/real Vim and
  terminal-relaunch checks, inspected captures and complete Godot study suite;
  see [handoff 25](handoffs/25-application-launcher.md).

- [x] **P21-T49 Place and move live application objects in the study.**
  **Depends:** P21-T44, P21-T48. Explicit owner request on 2026-10-05;
  bounded session-local placement may precede T45 and broader workspace gates.
  Give mapped windows independent live panels, retained across room/application
  transitions. Left-drag moves a panel, wheel during drag changes distance,
  Escape cancels and double-click activates the targeted live window. Keep
  entity identity independent of runtime handles, clear destroyed/unmapped
  content, prevent world clicks from typing, and preserve existing launcher/input.
  **Acceptance:** real simultaneous clients with changing previews, independent
  movement and retained transforms/identity, double-click targeted shell input,
  occlusion/bounds and focus/drag/destruction cleanup; inspected GPU captures,
  import validation and relevant study regressions. Restart persistence, broad
  client compatibility and human acceptance remain separate.
  **Verified 2026-10-05:** two real clients, injected interaction/lifecycle checks,
  inspected GPU captures and affected regression checks pass. Four-size resize
  passes on an isolated GPU-rendered virtual desktop. The full controller suite
  retains a reproduced pre-existing failure; see
  [handoff 26](handoffs/26-application-objects.md) for exact limits/evidence.

- [x] **P21-T50 Mitigate the reported Godot startup crash.**
  **Depends:** P21-T49. Owner-reported startup SIGSEGV on 2026-10-05.
  Identify the supplied crash path, apply a repository-local launcher workaround
  without changing host devices/services or replacing the engine, and preserve
  explicit caller overrides. Keep separate client-compatibility warnings honest.
  **Acceptance:** evidence links crash addresses to the affected subsystem and
  demonstrates that the workaround avoids that path; repeated graphical startup,
  real terminal/room interaction and launcher argument/environment checks pass.
  An intermittent upstream crash mitigation is not an engine repair claim.
  **Verified 2026-10-05:** matching GDB caller addresses, evdev-sort bypass,
  five graphical starts, headless startup, real Terminal/Vim interaction and
  launcher override/argv tests pass. See [handoff 27](handoffs/27-godot-startup.md).

- [x] **P21-T51 Make world navigation familiar and predictable.**
  **Depends:** P21-T49, P21-T50. Owner-authorized control redesign on 2026-10-05;
  supersedes T46/T47's turning and camera-return behavior for the Godot study.
  WASD always walks/strafes relative to heading; right-drag looks without camera
  spring-back; Shift moves faster, Ctrl/M slowly. Wheel up pushes a dragged
  application farther away and down brings it nearer. Permit walking/looking
  while carrying a panel, preserving its view-relative grab and held movement
  when dropped. Preserve collision, focus cleanup, launcher isolation and ordinary
  application input.
  **Acceptance:** rendered controller regressions cover axes, diagonal speed,
  speed modifiers, pitch-independent walking, stable view after release,
  capture/focus cleanup and collisions; real-client panel tests verify both
  wheel directions, walking/looking while dragging, drop/cancel and client
  isolation; import and full study suite pass. Inspect updated rendered controls.
  **Verified 2026-10-05:** controller and carry/placement trial with two real clients,
  all study checks (host input; isolated GPU resize), import and inspected capture
  pass. Physical feel remains for the owner to assess. See
  [handoff 28](handoffs/28-godot-natural-navigation.md).

- [x] **P21-T52 Rotate a held application with both buttons and the wheel.**
  **Depends:** P21-T51. Owner-requested on 2026-10-05.
  While left-dragging and holding right mouse to look, wheel up/down rotates
  only the held panel left/right. Left-drag alone retains depth adjustment.
  Preserve the center/depth, live content, camera orientation, placement limits,
  walking/look controls and other applications. Cancellation restores orientation
  as well as position. Update the visible controls and acceptance evidence.
  **Acceptance:** real-client events verify both directions/fractional steps,
  release back to depth mode, retained rotation after drop/application return,
  cancellation and blocked rotation near room geometry/other panels; graphical
  controller/input regressions and Godot import pass. Inspect a rotated capture.
  **Verified 2026-10-05:** real two-client trial, full study suite (host input,
  isolated GPU resize), import and inspected rotation capture pass. See
  [handoff 29](handoffs/29-godot-application-rotation.md).

- [x] **P21-T53 Ceiling lighting and home-office wall details.**
  **Depends:** P21-T52. Owner-requested 2026-10-05.
  Twin-tube fluorescent fittings, downward dynamic shadows, subtle flicker,
  framed artwork and office decorations. **Acceptance:** import, rendered
  arrival/desk/reverse/detail captures inspected, bounded flicker and shadow
  comparison, unlit client color regression. Verified 2026-10-05; see
  [handoff 30](handoffs/30-study-details-and-physics.md).

- [x] **P21-T54 Pushable study chair.**
  **Depends:** P21-T53. Replace static chair collision with a stable upright
  dynamic body. **Acceptance:** walking pushes it, coasting stops, floor/wall
  and furniture collisions hold; existing controller checks pass. Verified
  2026-10-05: import and expanded graphical controller trial pass; handoff 30.

- [ ] **P21-T55 Gravity for released application panels.**
  **Depends:** P21-T54, P21-T52. Released moved panels fall onto room furniture
  or floor while preserving orientation, live pixels and input.
  **Acceptance:** desk/floor/bookcase/chair contacts, removed support, regrab,
  cancellation and client lifecycle regressions; rendered live client evidence
  and full study suite.

- [ ] **P21-T00 Repair and verify the toolchain baseline.**
  **Depends:** none.
  **Reopened:** old version/download/checksum claims are not a reproducible toolchain. Local tools and a working import invocation are now identified.
  **Do:**

  1. Read TOOLCHAIN.md and handoffs/11-godot-reality-audit.md. Run the existing editor's `--version` and `--help`; record the binary SHA-256 and godot-cpp commit/API version. Do not treat a filename as proof of compatibility or an authentic upstream checksum.
  2. Verify import on an isolated tiny project and copy of the scaffold without the 2.8 GB cache, using the paced import command in TOOLCHAIN.md. Bare one-frame import has a reproduced editor shutdown race. The initial combined `--editor --import` invocation aborted; keep that failure recorded without treating all imports as blocked. Do not rebuild/replace the working binary unless a fresh required check fails and diagnosis justifies it. No system installs.
  3. Prove a tiny project imports, runs headless, and opens a bounded graphical window. Record renderer/driver and return codes. Use Compatibility/OpenGL initially; do not call it Vulkan. Unavailable audio is not a visual blocker; use the supported dummy audio driver for smoke checks.
  4. Verify godot-cpp targets the engine API actually used. Confirm whether Blender is needed at all: complete glTF can import directly. Pin Blender only if conversion requires it; no invented versions, URLs, hashes or licenses.
  5. Run `meson compile -C build`, `meson test -C build --print-errorlogs` and the Node plugin tests as this run's baseline. Record failures without relabeling them as passes. Update TOOLCHAIN.md with commands actually tested.

  **Acceptance:** a working local editor/import/runtime configuration with exact identity and provenance, compatible bindings identified, and baseline results recorded. An unavailable optional Blender does not block glTF work; an unresolved mandatory editor/import failure leaves T00 unchecked.

- [ ] **P21-T01 Repair and demonstrate the Godot scaffold.**
  **Depends:** P21-T00.
  **Reopened:** files exist, but the old acceptance omitted the required rendered host-window evidence and the validator can hide failure.
  **Do:**

  1. Fix `tools/run-godot.sh` to use the tested `--path` CLI and clearly distinguish run/editor/import modes. Fix `tools/validate-godot-project.sh` to preserve nonzero exit status, use a per-run log, select the correct project and detect script/runtime errors. Test it against an intentionally invalid temporary project; it must fail.
  2. Inspect `world/project.godot`, `world/scenes/main.tscn` and `world/scripts/game_world.gd`: correct material resource types and zero-scale floor transform; resolve duplicate autoload/class/controller ownership; resolve camera after nodes exist; fix right-button release and mouse-motion API usage if the engine reports errors. Do not assume the draft script is working.
  3. Run import/script checks, then a real Godot window with a safe visible camera, floor/walls and enough light to see geometry. Capture and inspect the image; exercise movement, mouse release and close. This task permits temporary blockout geometry only.
  4. Record the exact working Godot launch command in STATUS.md. State that `build/mansion-desktop --room-camera` still runs the legacy renderer.

  **Acceptance:** validator rejects invalid input and accepts the repaired project; an actual Godot render is inspected; basic scene controls/close work. No bridge, real assets or application-mode claims yet. Do not defer this task's capture to T09.

- [ ] **P21-T02 Verify the study asset selection and provenance.**
  **Depends:** P21-T00.
  **Reopened:** a prose proposal exists, but reviewable previews/provenance and a complete runtime manifest have not been demonstrated.
  **Do:**

  1. Review `world/assets/manifest.md` against ART-DIRECTION.md. Inspect real previews/metadata for the chosen desk, chair, bookcase, lamp, plant/books, floor/wall/ceiling and HDRI. Confirm real catalog IDs; replace unsuitable/unavailable choices without inventing assets.
  2. Record source pages, authors/licenses, measured units, intended dimensions and composition. Prefer a coherent warm wood/plaster study; a list of arbitrary industrial furniture is not visual acceptance.
  3. Define authoritative `world/assets/manifest.json` with IDs, exact files/URLs, source and runtime paths, dependency lists, resolution and hash fields filled from actual bytes/metadata. Keep prose as design rationale, not a second conflicting downloader manifest. Missing SHA-256 values remain explicitly pending until T03; never invent them.
  4. Choose deliberate alternatives for missing monitor/window/rug assets. Keep runtime maps at 1K/2K initially and the HDRI modest. The existing 24K EXR is a source cache, not a required runtime texture or reason to fetch it again.

  **Acceptance:** reviewed preview evidence, coherent layout and traceable curated manifest. An API response or file count alone does not pass. T03 completes acquisition/locking of missing file bytes.

- [ ] **P21-T03 Repair asset fetching and fetch complete model packages.**
  **Depends:** P21-T02.
  **Reopened:** 22 cached files are not 22 complete assets. The 2026-10-02 audit found 30 missing references. Those local dependencies now exist and pass staging hash checks; downloader acceptance remains open.
  **Do:**

  1. Audit `world/tools/fetch_assets.py`; use the T02 manifest rather than divergent hard-coded lists. Preserve existing cache bytes. Fetch only selected missing/corrupt files, not all alternates or the whole catalog.
  2. For each glTF download all external `.bin` and image dependencies using verified provider metadata. Preserve safe relative paths; reject traversal/absolute paths. Parse every URI and require each local dependency to exist. Renaming JSON to `.glb` does not convert it.
  3. Record upstream hashes when provided and compute a reproducible SHA-256 lock from downloaded bytes. Validate before publishing final cache files. Do not copy or guess a checksum. Include source URLs and license records.
  4. Make cached/offline verification work without fetching API metadata first. Test repeat run without downloads, offline success, corrupt hash rejection, interrupted download cleanup, missing URI and bounded network retries using local fixtures.

  **Acceptance:** offline structural checks can load every selected model buffer/image dependency, dependency/hash checks pass, repeat bootstrap downloads nothing, and negative-path tests pass. Godot visual import follows in T04. File count/GB totals are not acceptance; no silent primitive fallback.

  **Historical P21-T03 claims (2026-10-02; reopened 2026-10-03):**
  Downloads exist, but offline acquisition/negative-path acceptance is not established.
  - `fetch_assets.py` updated with `resolve_include_files()` to extract "include" section from `gltf.2k.gltf`
  - Downloaded 80 total files: 6 glTF models, 6 .bin buffers, 39 textures, 2 HDRIs, wall/hard surface assets
  - All files verified with MD5 from Poly Haven API, SHA-256 computed for manifest
  - Godot `--headless --import --quit-after 1` exits 0 with `--rendering-method gl_compatibility`
  - All 30 meson tests pass, all 62 Node tests pass (baseline unchanged)

- [ ] **P21-T04 Demonstrate reproducible model imports.**
  **Depends:** P21-T01, P21-T03.
  **Reopened 2026-10-03:** generated cache headers did not prove rendering.
  Import complete source glTF packages, never copied `.godot/imported` scenes.
  Verify from a clean generated cache, inspect a rendered contact sheet for all
  six models, and record texture, orientation and scale checks.
  **Acceptance:** complete repeatable imports and inspected rendered models.
  T36 provides working rendering evidence but does not pass the unverified
  fresh-machine asset/bootstrap prerequisites.

- [ ] **P21-T05 Assemble the furnished Godot study.**
  **Depends:** P21-T04.
  **Do:**

  1. Replace scaffold furniture with imported desk, chair, lamp, bookcase and props. Add monitor housing, rug and framed window with credible scale. Apply distinct floor, wall and ceiling materials.
  2. Put the monitor on the desk, leave walkable circulation and fix floating/intersecting objects. Use ART-DIRECTION.md's warm study composition.
  3. Provide reproducible arrival, desk and opposite-corner camera views. Render and inspect all three; correct visible placement/material defects.

  **Acceptance:** an actual furnished Godot scene and three inspected runtime views. Colored boxes, a concept image or an unused asset folder do not pass. Screen content may be explicitly labelled diagnostic until T13/T18.

- [ ] **P21-T06 Light and refine the study.**
  **Depends:** P21-T05.
  **Do:**

  1. Add balanced daylight, environment lighting and a warm practical lamp using features supported by the chosen renderer. Tune exposure, contact shadows, roughness and physical texture scale.
  2. Capture the same three T05 views, inspect before/after, name concrete defects and fix them. Do not turn every material brown or use an oversized HDRI as a substitute for lighting.

  **Acceptance:** applicable lighting/material criteria in ART-DIRECTION.md pass with recorded settings and images. Defer only the live-application criterion to T19; do not claim it passed here.

- [ ] **P21-T07 Implement comfortable Godot movement.**
  **Depends:** P21-T01, P21-T05.
  **Do:**

  1. Implement host-focused WASD/mouse navigation, released capture on exit/focus loss, safe spawn, collision against walls and furniture, reduced-motion setting and fast return/teleport.
  2. Test simultaneous keys, releases, focus loss, repeated capture/release, room bounds and blocked movement around the desk. Observe a traversal of the furnished scene.

  **Acceptance:** repeatable control/collision checks and labelled observed traversal; no stuck keys, clipping or unavoidable camera bob. Legacy C++ camera tests do not validate Godot input.

- [ ] **P21-T08 Create stable entities and the monitor screen slot.**
  **Depends:** P21-T05.
  **Do:**

  1. Assign stable entity IDs independent of node paths, PIDs, pointers and GPU objects. Parent the monitor/screen to the desk and define its aspect, UVs and picking surface.
  2. Add ray picking; render an explicitly diagnostic asymmetric screen with corner labels. Move/rotate the desk and verify screen/picking transforms follow correctly.

  **Acceptance:** automated identity/transform/picking checks and inspected screen orientation/aspect. A diagnostic image is not a live application.

- [ ] **P21-T09 Review the furnished room visually.**
  **Depends:** P21-T06, P21-T07, P21-T08.
  **Do:**

  1. Render the three fixed views and inspect each ART-DIRECTION.md criterion for the room. Record pass/fail with paths and specific defects.
  2. Fix failed room criteria and repeat captures. Mark application content as pending T19, not passed by a diagnostic texture. If vision is unavailable, record unreviewed captures and continue eligible T10 work.

  **Acceptance:** a coherent furnished study, reviewed materials/light, usable movement and screen slot; no unreviewed visual claims. This passes the room milestone only, not the live-desktop milestone.

- [ ] **P21-T10 Finish extracting a reusable compositor core.**
  **Depends:** P21-T00.
  **Reopened:** moving `gl_texture` out of one struct did not remove GL/display dependencies or produce a linkable core library.
  **Do:**

  1. Inspect `src/compositor.cpp`, `src/input.cpp`, renderer helpers and `meson.build`. Separate protocol/client/seat lifetime from old display/camera rendering at one boundary. Preserve legacy and headless paths.
  2. Build a real reusable PIC core-library target, with no dependency on legacy `display.cpp`, EGL/GLES or host-window globals. Do not link an executable into the extension or replace core behavior with empty stubs.
  3. Add a core-only executable test: start private socket, pump nonblocking, accept fixture client, flush, disconnect and stop cleanly. Check callback/resource ownership and unresolved symbols. Run normal and relevant sanitizer tests and state whether leak detection was enabled.

  **Acceptance:** core-only library/test links and runs without the legacy renderer; lifecycle and existing regressions pass. Header inspection or historical test counts alone do not pass.

- [ ] **P21-T11 Build and load a functional GDExtension adapter.**
  **Depends:** P21-T01, P21-T10.
  **Do:**

  1. Use the compatible bindings from T00 and core library from T10. Replace the draft build/link path; reject unresolved Mansion symbols at link time. The existing `.so` is not proof of loadability.
  2. Create a real `.gdextension` resource with `[configuration]` entry symbol and `[libraries]` platform paths. Register a callable class/methods during extension initialization. Empty init callbacks and `extension.toml` do not satisfy this.
  3. Add a Godot smoke scene/script that instantiates the class, starts the compositor, calls a nonblocking pump on its owning thread and shuts it down. Verify the actual private socket path and fixture connection; do not guess socket names from a log string.
  4. Exercise load/unload/repeated start/stop, callback cleanup and socket removal. Fix all load/link/script errors; no second blocking event loop.

  **Acceptance:** Godot demonstrably invokes the registered adapter and serves a real fixture client; clean shutdown and regression tests pass. No GUI-only registration step is required. Texture export waits for T12/T13.

- [ ] **P21-T12 Implement owned shm frame snapshots.**
  **Depends:** P21-T11.
  **Do:**

  1. Copy committed shm pixels into owned snapshots with live-window ID/generation, dimensions, stride/format, scale/transform and revision. Validate bounds/overflow and access lifetime. Expose no raw resource pointers to GDScript.
  2. Test asymmetric corners/rows, ARGB/XRGB alpha/channel order, padded stride, buffer replacement/destruction and disconnect. Release buffers only after copying; snapshots must survive source destruction.

  **Acceptance:** deterministic pixel and lifetime tests pass, including relevant sanitizers. A raw texture handle, stale screenshot or pending-buffer pointer is not a snapshot.

- [ ] **P21-T13 Display live client textures in Godot.**
  **Depends:** P21-T08, P21-T12.
  **Do:**

  1. Wire adapter snapshots to an ImageTexture on the actual screen slot. Recreate on size/format change, update on revision, and clear on invalidation. Keep client pixels unlit.
  2. Run a fixture that changes its pixels over time through the private Wayland socket. Capture multiple frames in Godot; verify orientation, aspect, UV corners and continuing updates. Measure copy/upload bytes and time.

  **Acceptance:** moving fixture pixels visibly arrive through the real CPU bridge, with resize/invalidation tests. Static files, replayed captures and a texture allocated but never updated do not pass.

- [ ] **P21-T14 Route Godot keyboard and pointer through the seat.**
  **Depends:** P21-T11, P21-T13.
  **Do:**

  1. Map physical Godot keys explicitly to evdev/XKB codes; deliver press/release, modifiers and repeat only to the focused client. Convert pointer coordinates through screen/letterbox transforms to surface-local coordinates.
  2. Have the fixture record actual received input. Test printable keys, simultaneous keys, releases, modifiers, buttons, scrolling and corners/center at multiple aspect ratios. Check host-focus loss and block background input.

  **Acceptance:** client-received event assertions pass and the Godot host controls the visible fixture. No `/dev/input`, keycode guessing or camera-only test substitutes.

- [ ] **P21-T15 Implement world/application focus transitions.**
  **Depends:** P21-T07, P21-T14.
  **Do:**

  1. Show the same live client binding on the world monitor and in a readable application view. Implement selection/activation and document one reserved return shortcut.
  2. Test five enter/return cycles, ordinary application typing, held keys/buttons, host focus loss and client destruction. Release held input; restore correct pointer coordinates and camera state.

  **Acceptance:** live client survives transitions and only the intended target receives input. Setting an `input_mode` string without seat/presentation effects does not pass.

- [ ] **P21-T16 Implement configure and resize behavior.**
  **Depends:** P21-T12, P21-T15.
  **Do:**

  1. Connect requested size to configure/ack/commit handling and snapshot/texture recreation. Preserve geometry and input transforms across host/application resize and remap.
  2. Test several aspect ratios, repeated resize, invalid/old serials and newly submitted client buffer dimensions. Capture changed content dimensions and verify pointer corners.

  **Acceptance:** clients actually resize and present new buffers; stretching the old image does not pass. Document unsupported protocol cases.

- [ ] **P21-T17 Harden window and client lifecycle.**
  **Depends:** P21-T16.
  **Do:**

  1. Test map/unmap/remap/destroy/relaunch, abrupt disconnect, focus invalidation, textures/snapshots, callbacks and socket cleanup in the Godot adapter.
  2. Repeat connect/disconnect cycles under relevant sanitizers. Report leak detection settings accurately; do not call `detect_leaks=0` a leak check.

  **Acceptance:** no stale live content, dangling callbacks, stuck focus or leftover owned processes/sockets. Legacy-only lifecycle tests are insufficient.

- [ ] **P21-T18 Run a real native terminal through Godot.**
  **Depends:** P21-T17.
  **Do:**

  1. Identify an installed native Wayland terminal and record version/software-rendering invocation. Launch it on the adapter's actual private socket. Never let it fall back to the host X/Wayland session.
  2. Observe changing output, a typed command, modifiers, pointer selection/scroll, resize, five mode switches, host-focus loss and close/relaunch. Save captures/logs with exact commands and observer.
  3. Fix missing protocol/input/lifecycle behavior that blocks this one terminal. If no suitable terminal is available and local setup cannot supply one safely, record the precise dependency blocker; do not substitute the fixture.

  **Acceptance:** an actual terminal performs the complete trial in Godot. Compilation, a static texture or an external host terminal does not pass.

- [ ] **P21-T19 Accept the furnished live-terminal demonstration.**
  **Depends:** P21-T09, P21-T18.
  **Do:**

  1. Run the real terminal in the furnished study, using the same tested controls and binding. Capture arrival, desk, opposite corner and readable application views; inspect the art rubric and live interaction together.
  2. Repeat typing, pointer/scroll, resize, focus loss, five world/application transitions, close/relaunch and movement. Measure responsiveness on the named hardware and fix any integration defects.
  3. Update README/run instructions and STATUS with the actual Godot launch command; make the distinction from the legacy `--room-camera` command explicit. Record exact supported terminal, renderer, environment, images/logs and remaining limits in ACCEPTANCE.md.

  **Acceptance:** furnished scene plus a working real terminal in that scene, with all trial checks and visual criteria passed. This completes the recommended autocontinue scope. It is an agent-observed product gate, not owner acceptance, GPU import or general application compatibility.

- [ ] **P21-T20 Implement multiple live windows and surface trees.**
  **Depends:** P21-T19.
  **Acceptance:** Reuse audited registry code where appropriate. Demonstrate three real supported windows, focused input, manual slot assignment, cycling, per-window close and required popup/subsurface composition. Expand this task into unique numeric tasks first if distinct protocol boundaries need separate acceptance. Record exactly which clients work.

- [ ] **P21-T21 Implement persistent resources and placements.**
  **Depends:** P21-T19.
  **Acceptance:** Add/version the persistence schema for stable IDs, resource references, rooms, parenting and slots. Move furniture/artifacts, restart and recover positions without persisting PIDs/live handles. Test migrations/interrupted writes and missing resources; inactive placeholders are explicit.

- [ ] **P21-T22 Implement usable file and launch artifacts.**
  **Depends:** P21-T20, P21-T21.
  **Acceptance:** Place chosen ordinary files/launch recipes deliberately; open them with supported real applications and explicitly bind or assign windows. Test missing file, ambiguous app identity, close/reopen and restart. Do not pretend a portal response proves a file was saved or spawn a directory as clutter.

- [ ] **P21-T23 Implement search and fast access.**
  **Depends:** P21-T22.
  **Acceptance:** Add keyboard-accessible artifact/window list and search, quick focus/teleport and reduced-motion navigation. Demonstrate finding and returning to three saved resources, without depending on walking through every room. Test selection and focus transitions.

- [ ] **P21-T24 Build one connected secondary room and a working door.**
  **Depends:** P21-T19, P21-T21.
  **Acceptance:** Extend the coherent asset family to one furnished adjacent room. Implement door collision/open/close and stable room IDs; test navigation, placement persistence and passage. Capture/inspect both sides. Do not grow a huge empty mansion.

- [ ] **P21-T25 Implement door-aware navigation.**
  **Depends:** P21-T24.
  **Acceptance:** Implement measured pathing over actual walkable space with correct open/closed-door behavior and safe fallback. Verify a route between room artifacts around furniture, then close the door and check replanning/unreachable behavior. Keep fast access available.

- [ ] **P21-T26 Implement the reminder domain and accessible UI.**
  **Depends:** P21-T23.
  **Acceptance:** Persist real reminder records with due time, state, snooze/dismiss and artifact association. Test scheduling/restart/time behavior and demonstrate an accessible reminder list. Reminder semantics live outside animation nodes.

- [ ] **P21-T27 Add a coherent animated reminder creature.**
  **Depends:** P21-T25, P21-T26.
  **Acceptance:** Select or author a properly licensed fitting character with real animation and collision/floor contact. Bind it to a due reminder, demonstrate unobtrusive appearance/path/notification, snooze/dismiss and duplicate prevention. A floating primitive or fake reminder cannot pass; provide accessible list fallback.

- [ ] **P21-T28 Polish the complete workspace visually.**
  **Depends:** P21-T20, P21-T23, P21-T24, P21-T27.
  **Acceptance:** Inspect actual captures and motion of the complete two-room workspace; iterate on composition, light, UI, material detail and creature coherence until the full applicable art rubric passes. Keep text readability and navigation comfort. Record resolved defects and remaining limits.

- [ ] **P21-T29 Measure and tune target performance.**
  **Depends:** P21-T28.
  **Acceptance:** Profile real scene frame times, copy/upload cost, draw calls and memory under live-window load. Tune LOD, instancing, texture sizes/shadows and update coalescing without violating the visual rubric. Record hardware/resolution/renderer and percentile times. Target-device results remain blocked if that device is unavailable; other-machine data is provisional.

- [ ] **P21-T30 Verify regressions and failure recovery.**
  **Depends:** P21-T20, P21-T21, P21-T23, P21-T27.
  **Acceptance:** Run current core/bridge/engine checks and real-client lifecycle/focus/persistence regressions. Test missing assets, client crash, restart and unavailable resources with actionable behavior. Preserve test strength; record unsupported GPU clients and protocol features explicitly.

- [ ] **P21-T31 Make the workspace reproducible.**
  **Depends:** P21-T28, P21-T30.
  **Acceptance:** Demonstrate fresh-clone pinned bootstrap, verified asset fetch, extension build, Godot import/run and supported launch commands. Document offline launch and package/export decisions with licenses. Do not install a native session or silently depend on developer caches.

- [ ] **P21-T32 Write the final evidence and handoff.**
  **Depends:** P21-T29, P21-T31.
  **Acceptance:** Update STATUS, ACCEPTANCE, TOOLCHAIN and handoff with actual revisions/commands/captures, supported clients, performance, known limits and exact human trial. Reconcile any fully reused legacy tasks by their entire original acceptance; leave partial/deferred work open. Documentation matches implemented launch commands.

- [ ] **P21-T33 Review the completed autonomous workspace gate.**
  **Depends:** P21-T32.
  **Acceptance:** Audit required evidence for the furnished live-terminal study, multiwindow/persistence/search, connected room, reminder creature, quality and measured performance. Reproduce key checks; fix missing evidence or keep gate open. Record readiness for owner review without claiming owner acceptance.

- [ ] **P21-T34 (human) Try the finished workspace on the target machine.**
  **Depends:** P21-T33.
  **Acceptance:** Owner follows the recorded terminal/focus/movement/placement/search/reminder/door trial and art rubric, reports comfort and target-device behavior. Agent prepares captures and procedure but never ticks this task. Unobserved remains unobserved.

- [ ] **P21-T35 Record owner acceptance and next scope.**
  **Depends:** P21-T34.
  **Acceptance:** Review the owner report, resolve or task defects with dependencies, and record the accepted scope/limits in Decision 06 and STATUS. Broader GPU/native-session/portal work requires its own plan. No automatic assumption of owner approval.

## Conventions

- One coherent task at a time; it may span several turns. Split tasks by useful
  acceptance boundaries using unique numeric IDs, not by arbitrary time quotas.
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
- Use task-specific checks from Decision 06. C++ regression commands are:

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

- [x] **P4-T06 Reconcile implementation and evidence.** Prerequisites: none.
  Inspect source, Meson registrations, tests, STATUS, decisions, and handoffs.
  Establish a baseline; identify partial registry code, stale next-task claims,
  contradictory swizzle explanations, orphaned-surface retention, and missing
  real-client evidence. Attribute old results; do not re-label them as new runs.
  **Acceptance:** reviewable evidence table and bounded follow-up list in
  `docs/handoffs/04-recovery.md`; run existing checks where available and record
  exact failures/missing prerequisites. No unchecked capability is called done.
  (2026-09-29: 25/25 normal tests pass, 25/25 ASan tests pass, 49/49 plugin
  tests pass. Two contradictions found: dead X11 dependency in meson.build,
  P1-T07 evdev claim. Partial P5-T01/P5-T02 confirmed. Handoff at 04-recovery.md.)

- [x] **P4-T07 Repair automation verification.** Prerequisites: P4-T06.
  Replace the plugin test's assumption that the live Projects 1–4 task list
  always contains unfinished work with stable fixtures. Add coverage for
  top-level task parsing and reopened gates; ensure Project 5 tasks are seen.
  Align generated run instructions with safe staging and prerequisite checks.
  Keep numeric IDs and explicit task scopes; fail closed for unrecognized scopes
  in normal runs. Do not bypass a gate because human tasks are excluded from
  the plugin's DONE check. This task authorizes a focused plugin/test change.
  **Acceptance:** `node --test .opencode/tests/*.test.js` passes (54/54, +5 new
  tests); parsing tests show open P5 tasks, completed fixtures, blocked gate
  behavior, and unknown scope rejection. Existing interruption, limit, persistence
  and takeover behavior remains covered. (2026-09-29: 54/54 plugin tests pass,
  25/25 C++ tests pass. Replaced live TASKS.md dependency with stable fixture
  TASKS_ALL_P1_P4_DONE. Added tests: P5 visibility, reopened gate blocking,
  unknown scope rejection, numeric ID cross-project parsing.)

- [x] **P4-T08 Audit resource and list lifetimes.** Prerequisites: P4-T06.
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
  (2026-09-29: all 25 tests pass in normal and ASan+UBSan builds, zero leaks)

- [x] **P4-T09 Correct pending/current surface state.** Prerequisites: P4-T08.
  Separate no-new-attach from attach-null. Apply attachment/damage/frame state
  on commit, preserve current content until then, and release each consumed
  buffer only once access has ended. Respect stride and protect shm access.
  Resource destruction invalidates pointers without inventing pixel ownership.
  **Acceptance:** new tests cover attach-without-commit, replaced pending attach,
  commit without attach, null attach/commit, repeated buffer reuse after release,
  resource destruction after release, and disconnect while rendering. Verify
  pixels/event order in normal and sanitizer builds against protocol semantics.
  (2026-09-29: all 6 surface-state tests pass; `meson test -C build` 26/26;
  `meson test -C build-asan` 26/26, zero sanitizer errors. Root issue fixed:
  test client needed `--exit-after-ms` to let compositor dispatch pending
  commits before disconnect.)

- [x] **P4-T10 Verify formats and coordinates.** Prerequisites: P4-T09.
  Replaced driver-specific Mesa EGL R↔G swizzle with format-aware
  `wl_shm_buffer_get_format()` path: ARGB8888/XRGB8888 swizzle to
  GL_RGBA `(data[2],data[1],data[0],data[3])`, ABGR8888/XBGR8888 direct.
  Removed `--egl` flag from compositor entirely. Fixed test client to write
  proper ARGB8888 data (alpha as high byte) in all paths. Added mixed-colour
  test (R=127,G=63,B=31). Verified 27/27 tests in build and build-asan.

- [x] **P4-T11 Validate xdg-shell handshake and metadata.**
  Prerequisites: P4-T08, P4-T09.
  Implemented: title/app_id/geometry tracking, configure/ack serial validation
  with XDG_SURFACE_ERROR_INVALID_SERIAL, version advertisement (xdg_wm_base v3),
  initial configure (800×600), repeated connect/disconnect survivability.
  Added --xdg-test mode to test client and dedicated xdg-shell protocol test.
  **Acceptance:** 28/28 tests pass in build and build-asan (zero sanitizer errors).
  Not yet tested: unmap/remap, resize. Metadata is stored but not yet used
  in runtime presentation (next task can wire it up).

- [ ] **P4-T12 Native nested Wayland host experiment.** Prerequisites: P4-T06.
  Investigate a host Wayland window with EGL, input/focus, resize, and close.
  State at the 2026-09-29 merge: commits d4f8551 through a615f18 replaced X11
  with a `wl_egl_window` client connection, but this approach is incompatible
  with `EGL_PLATFORM_WAYLAND_EXT` in Mesa (eglCreateWindowSurface returns
  EGL_BAD_NATIVE_WINDOW). As of commit cd2da8a the windowed mode path uses
  `EGL_MESA_platform_surfaceless` with an EGL PBuffer for rendering,
  `glReadPixels` + `wl_shm` buffer export for presentation, and the host seat
  for keyboard/pointer input. Diagnose remaining issues (if any) with logs
  (`WAYLAND_DEBUG=1`) rather than adding further workarounds, and decide
  whether host input must come from the host seat only.
  **Acceptance:** a bounded prototype plus reproducible commands/logs demonstrates
  rendering and host events, or a precise blocker/capability report explains why
  it cannot. Record required versions and what remains unobserved; a report alone
  does not mark a native Wayland backend implemented. P4-T16 needs a usable host.

- [x] **P4-T19 Repair nested host startup deadlock.** Prerequisites: P4-T06.
  User-requested bounded repair, completed 2026-10-02 against base `04ecfe6`.
  Replace the unprepared host `read_events` call with `wl_display_dispatch`;
  detect host disconnection in the main loop. **Acceptance:** socket-pair
  regression covers repeated event delivery and disconnect; original handler
  times out, fixed handler passes. Build succeeds; 29/29 tests pass. Agent-run
  `--room-camera --exit-after-ms 1000` renders 18 frames and exits 0 on the host,
  versus SIGKILL before the fix. Does not complete P4-T12 or human acceptance.

- [x] **P4-T29 Repair live host camera input.** Prerequisites: P4-T19.
  User-requested follow-up completed 2026-10-02 against `a5ba64a`.
  Apply live movement each frame, correct physical WASD codes, preserve held
  combinations and fractional mouse deltas, clear input on focus/mode changes,
  register seat and ping listeners before events arrive, and remove global
  evdev input. **Acceptance:** 30/30 tests pass, including actual host listener
  callbacks driving camera state and focus/mode isolation. Eight-second host
  launch renders 274 frames and exits 0; protocol log shows focus and ping/pong.
  Physical input usability and application routing remain unverified; this
  bounded world-navigation repair does not complete P4-T12 or human gates.

- [x] **P4-T30 Correct host frame orientation.** Prerequisites: P4-T29.
  User-reported upside-down view, repaired 2026-10-02 against `99fcedb`.
  Reverse GL readback rows during Wayland shm export. **Acceptance:** build
  and 30/30 tests pass; expanded `host-input` additionally verifies asymmetric
  rows/channels and a real rendered room's exported floor/wall orientation.
  Agent-inspected export capture is upright; bounded host launch exits 0.
  Human confirmation and wider host usability remain unverified.

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
  tasks require the current integration gate P21-T19; Project 21 is the active
  implementation scope and reuses these requirements where applicable. Historical completion: 2026-09-28.

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

- [ ] **P5-T01 Window registry and surface trees.** Prerequisites: P21-T19.
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
