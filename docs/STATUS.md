# Project status

Current authority: owner-requested navigation/carrying T51 verified 2026-10-05
from `c789f37`; recovery begun 2026-10-03; T44 and navigation/launcher T46–T48
verified 2026-10-04; owner-requested application objects T49 verified 2026-10-05,
starting from `4c0c078` plus the existing
uncommitted overnight work. [Decision 06](decisions/06-godot-poly-haven.md) remains
the direction. [Previous status](handoffs/15-status-before-frontend-recovery.md)
is historical; its extension/visual completion claims were contradicted by tests.

## Next task

**P21-T45: finish targeted client close and its lifecycle acceptance.**
Recovery T36–T44 passes its bounded acceptance: furnished study, retained
compositor, live output, typing, selection/scroll and actual client resize.
See [tasks](TASKS.md) and [resize handoff](handoffs/23-godot-terminal-resize.md).
The owner's latest navigation and application-carrying controls are recorded in
[handoff 28](handoffs/28-godot-natural-navigation.md); they supersede the earlier
[handoff 24](handoffs/24-godot-navigation.md) controls and do not complete T45.
The requested application wheel/search and explicit terminal relaunch now pass
T48; see [handoff 25](handoffs/25-application-launcher.md). Reuse that launcher.
The owner-requested movable live application panels are recorded in
[handoff 26](handoffs/26-application-objects.md). This bounded request does not
start a broader roadmap run. Next send targeted xdg close requests without using
process termination as a window-close implementation. Cursor images, clipboard
and popup menus remain unsupported; carry these into concrete follow-up tasks.
Broader T00–T19 gates remain unchecked until their full evidence exists.

## Run the visible Godot frontend

```sh
python3 world/tools/prepare_study_assets.py
tools/validate-godot-project.sh
tools/run-godot.sh --audio-driver Dummy
```

Uses the existing local engine and downloaded source cache. Missing/corrupt model
dependencies fail explicitly. Fresh-machine bootstrap remains unfinished.
`tools/run-godot.sh --editor` opens the editor. The default runs the study.
The launcher builds/stages the standard native extension and starts installed
`/usr/bin/weston-terminal` with `/bin/sh`. For a changing clock/counter, use
`tools/run-godot.sh --audio-driver Dummy -- --terminal-demo`.
The launcher now defaults to SDL's classic Linux joystick path to avoid the
reported pre-render startup crash. Explicit `SDL_JOYSTICK_LINUX_CLASSIC` overrides
are preserved; see [startup evidence](handoffs/27-godot-startup.md). This is a
tested workaround for the bundled engine, not an upstream engine repair.
`build/mansion-desktop --room-camera` still uses the separate legacy renderer.

WASD walks relative to your heading; A/D always strafe. Hold right mouse to
look (mouse right turns right, mouse up looks up); release to free the pointer.
Your view stays where you aimed, with no automatic return. Shift moves faster;
hold Ctrl or toggle M for slow walking. Slow mode takes priority over Shift.
Escape stops movement and frees the pointer; Home stops and resets to arrival.
Enter activates the terminal. Ctrl+Alt+Escape returns to the room. Application
mode uses physical US keys and native-size text when it fits; ordinary Escape,
Tab, Home and camera keys reach the terminal. Host focus loss returns to world
mode. Left-drag selects text and the wheel scrolls terminal history. Type `exit`
to close the shell; use the application wheel to relaunch. In application mode,
resize the host window to resize the actual terminal and its rows/columns.

Tab in world mode (or the Applications button) opens eight recent-app slots.
Newest is at twelve o'clock, then clockwise; unused slots stay empty. Click the
center to search installed application names/keywords with their icons. Enter,
double-click or Launch starts the selected app; a recent live app is restored
without duplicating it. Esc returns from search to the wheel, then to the room.
Tab still belongs to applications in application mode. Recents persist in
`.tools/launcher-state/recent.json`; tests use an isolated state directory.
Every mapped window also has a separate live panel in the room. Single-click
selects it; double-click activates it. Left-drag moves the panel. Keep holding
left mouse to carry it while using WASD and right-mouse look; dropping it does
not interrupt held walking keys. Wheel up pushes it farther away, wheel down
brings it nearer. Escape cancels the move and stops navigation.
Panels retain their positions when returning with Ctrl+Alt+Escape, and background
applications keep updating. Placement lasts for this session; closing a client
removes its panel. Restart restoration is not implemented.

Discovery uses freedesktop entries and icon themes without a KDE service.
Verified clients are Weston terminal and Vim through Terminal=true. Other
graphical apps may fail; Flatpak launch is explicitly unavailable for now.

## Verified by automated checks in this recovery

- T51 (2026-10-05): consistent WASD strafing, normal mouse-look without camera
  return, slow/fast speeds, diagonal normalization, level walking while pitched,
  collision, GUI release and focus cleanup pass. Real Terminal/Vim tests pass
  walking/turning while carrying, continued movement after dropping, both wheel
  directions and fractional steps, cancellation, bounds and input isolation.
  Import and the full study suite pass using host input and isolated GPU-rendered
  four-size resize. Updated controls/panels were inspected in a 1280×800 capture.
  Physical mouse feel and owner comfort remain unobserved; see handoff 28.

- T50 (2026-10-05): supplied crash addresses matched SDL's evdev joystick sort;
  GDB confirms the launcher's classic-backend default avoids that branch.
  Five graphical starts, headless startup, real Terminal/Vim object interaction,
  and launcher environment/argv tests pass. Exact intermittent trigger remains
  unproven. The earlier `bssh` GTK-seat warnings remain a separate unverified
  client compatibility issue.
- T49 (2026-10-05): real Terminal and Vim coexist with independent live textures,
  entity IDs and placements. Injected drag/wheel, targeted double-click typing,
  cancellation, occlusion, room bounds and client-exit cleanup pass. GPU captures
  inspected; import, existing color/output/keyboard/pointer/launcher checks pass.
  Four-size resize plus world-panel pixel dimensions/aspect pass in an isolated
  GPU-rendered Weston virtual desktop. At that trial the host was portrait
  800×1280, so its width clamp prevented the 900–1280px resize test from passing.
- Historical T49 full-suite limitation: the then-current controller failed
  `Pitch return fights mouse after re-grabbing`. T51 removes camera-return
  behavior at the owner's request, replaces those expectations with persistent
  view regressions, and passes the full suite. Original evidence stays in
  [handoff 26](handoffs/26-application-objects.md).

- Six glTF packages staged with their manifest-checked buffer/texture dependencies.
- Godot import and runtime pass; validator rejects an invalid-script fixture.
- Graphical scene smoke: 56 textured surfaces, walking/key release, wall
  collision, focus cleanup, mouse capture/look/release and Home assertions pass.
  These are injected controller events, not a physical-input or human trial.
- Navigation T46 (2026-10-04): reversed left/right look, upward/downward pitch
  return, preserved yaw/held movement, interrupted return and Home reset pass in
  the graphical controller test. Physical mouse feel remains owner-unobserved.
- T47 navigation rechecked during T48: A/D turn by default and both strafe with
  right mouse held. The in-world help now describes these controls accurately.
- T48 launcher: six Python metadata/Exec/environment tests, actual center-click
  and typed search, camera/input isolation, cancel, eight-slot ordering,
  deduplicated persistent history, real Vim launch/typing, recent-window switch,
  terminal exit/relaunch with a shell-written marker and owned child cleanup pass.
  The complete Godot study suite passes in this session. Eight-entry visual
  history is a labeled layout fixture; only Terminal/Vim launch compatibility is
  claimed. See [launcher evidence](evidence/application-launcher/).
- Standard C++ binding: fresh import and three clean runtime processes, 300
  native instances/calls/releases. Separate build uses consistent generated headers.
- Real Wayland window protocols: sixteen standalone and sixteen Godot trials.
  Initial configure/ack/buffer commit, seat v1/v4 capabilities, parsable keymap,
  version-correct events/releases, partial destruction, disconnect, server stop
  and five invalid request sequences pass. Godot frames advance; resources,
  socket and private directory are cleaned up.
- Owned frame transport: 32 standalone + 32 Godot pixel/state barriers across
  repeated sessions. Exact RGBA/alpha/padding, buffer reuse/destruction, pending
  state, detach/remap, scale/transform metadata, callbacks and handle invalidation
  pass. Six malformed/unsupported buffer/state cases are rejected in each harness.
  These checks alone do not establish rendered usability.
- Live output: two real Weston terminal sessions, changing pixels/revisions,
  aspect ratio, unchanged-frame skip, disconnect clearing, child/socket cleanup
  and unchanged host display environment pass. Eight inverse image transforms
  and eleven GPU color patches under strong colored lighting pass.
- Keyboard protocol: focused delivery to two independent clients, same-client
  focus switches, US key/modifier values, duplicate suppression, held/locked
  state reset, late binding, detach, destruction and stale handles pass.
- Real shell input via injected Godot events: mixed case/punctuation, Backspace,
  Ctrl+U, client repeat, Ctrl+C interrupt, mode transitions, resumed world
  movement, host-focus-loss notification and shell exit pass. Flat view preserves
  7,571 sampled opaque client pixels at native size. These are agent-run trials.
- Pointer protocol: client isolation, fractional coordinates, button/axis values,
  implicit grabs, duplicate suppression, outside drag, late binding, reset,
  detach, destruction, disconnect and stale handles pass. 302 Godot assertions
  cover drawn-rectangle mapping, scale/rotation, corners/margins and buttons.
- Real Weston pointer trial via injected Godot events: text selection, release
  outside the image, scrollback up/down, held-button mode exit, world control
  recovery, focus-loss notification and disconnect pass. Captures were inspected;
  physical mouse/touchpad input and latency are not inferred.
- Resize protocol (T44, 2026-10-04): configure/ack/commit separation, retained
  snapshots during delay/decline, min/max limits, committed geometry, client-chosen
  sizes, serial validation, bounded pending requests and destruction pass.
  Native resize AddressSanitizer + UBSan + leak detection also pass.
- Real Weston resize: four host sizes (1000×700, 1200×780, 900×620, 1280×800)
  change character grids to 77×22, 95×25, 68×18, 103×26. Typing/selection continue,
  8,573 sampled opaque pixels match native-size GPU views, and the world preview
  updates. These are agent-injected window/input checks, not physical drag-resize.
- Standalone pointer runtime AddressSanitizer + UBSan + enabled leak detection
  pass in T43.
- Standalone keyboard runtime AddressSanitizer + UBSan + enabled leak detection
  pass in T42; frame ownership sanitizer checks passed in T40.
- C++ build and 33/33 Meson tests pass; 62/62 Node plugin tests passed during T39.
  The Meson/plugin checks alone do not validate the Godot integration.
- The local editor's first-import shutdown race has a tested paced-startup
  workaround, not an engine fix; see [TOOLCHAIN.md](TOOLCHAIN.md).

## Agent-observed visuals

[Three rendered views](evidence/godot-recovery/) show real desk, chair, lamp,
plant, books and shelving, room textures, daylight/shadows and the earlier inactive
monitor preview. Captures were inspected, including correction of the initially
backward desk/chair. Compatibility/OpenGL on AMD Custom GPU 0405.
This passes visible frontend recovery, not final art acceptance.

[Live terminal captures](evidence/live-terminal/) now show real Weston 15.0.1
output on the monitor, upright text, changing UTC clock/counter and clearing on
disconnect. Agent inspection: 2026-10-03, Godot 4.7.2, Mesa 26.2.4, 1280×800.
Room and close-up GPU views were inspected; `client-frame.png` is a source frame,
not a render. This is output acceptance, not typing or human readability acceptance.

[Application-mode evidence](evidence/terminal-input/) shows real command results
and return to the room after shell exit. Agent inspection, 2026-10-03, same
engine/renderer/hardware as above. Text is upright and comfortably sized in this
capture; no human, physical-keyboard or latency acceptance is inferred.

[Pointer evidence](evidence/terminal-pointer/) shows a highlighted real output
line, earlier/later scrollback and return to the live study. Agent inspection,
2026-10-03, same engine/renderer/hardware. This is selection/scroll acceptance;
it does not establish clipboard transfer or popup-menu support.

[Resize evidence](evidence/terminal-resize/) shows four real terminal sizes,
matching selections and the updated world monitor. Agent inspection, 2026-10-04,
same renderer/hardware, at the recorded host viewport sizes. Text remains upright
and at native size. Physical host resizing and human comfort remain unobserved.

## Verified by a person

Historical owner confirmation on 2026-10-02: legacy launch, input and vertical
orientation fixes worked. Owner response on 2026-10-03 to the recovered view: “finally! that looks great.”
This records approval of its appearance, not a terminal/input/comfort trial.

## Not verified / unfinished

- Compositor close requests. In-app terminal relaunch now passes T48. Keyboard,
  selection/scroll, resize and ordinary shell exit pass; full terminal
  usability and physical input are not verified. IME/composed text and selectable
  layouts are not implemented; the current seat/mapping is US.
- General graphical-app launch compatibility and durable/multiwindow assignment.
  Weston editor failed because text-input-manager is absent. The launcher records
  an app only after mapped content; a catalog entry is not a compatibility claim.
  Flatpak handoff and D-Bus activation remain unsupported. Third-party single-
  instance IPC may reuse a host instance; private display variables are not a
  process sandbox. The verified Terminal/Vim paths create owned private clients.
- Client cursor images, clipboard/primary selection and popup menus. The host
  cursor remains visible; highlighting does not implement copy/paste. A right-click
  menu request can disconnect the client because xdg_popup remains unsupported.
- Original addon initialization: reproduced segmentation fault; isolated with
  `world/addons/mansion_godot/.gdignore`, source and binary preserved.
- Full renderer-independent compositor extraction: socket/wl_compositor/shm
  lifecycle, initial xdg-shell/seat checks and owned shm snapshots pass. Broader
  surface-tree/protocol conformance and popup presentation remain open.
- Finished art rubric, furniture selection/cohesion, performance, physical host
  input and human comfort. No blank capture counts as visual acceptance.
- Fresh-clone dependency provenance and asset fetching/offline negative checks.

The earlier recovery preserved overnight edits and saved this local backup:
`.tools/recovery/20261003T185319Z/`. Never sweep them into a recovery commit.
