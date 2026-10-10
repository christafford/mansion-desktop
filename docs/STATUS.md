# Project status

The project is now **elsewhere**, hosted at
[christafford/elsewhere](https://github.com/christafford/elsewhere).
The owner-requested P21-T67 rename covers maintained text, identifiers and paths;
see [handoff 38](handoffs/38-elsewhere-rename.md). The native binary is
`build/elsewhere`, and environment overrides use `ELSEWHERE_`.

Current authority: basic Chrome and surface-tree T65–T66 verified 2026-10-09
from `2cdfd15`. Grand foyer T64 verified 2026-10-09 from `817aff3`;
merged exploration wings T63 verified 2026-10-09 from `95ea216`. Longer rooms
and movable furnishings T62 were verified
2026-10-08–09 from `58d2be5`. Exploration rooms T61 were verified 2026-10-08
from `e1deea6`. Hallway T60 was verified 2026-10-08 from `139214d`.
Furnishings, walls, instances and transitions T56–T59 were verified
2026-10-06, starting from `e618487`.
Previous authority: owner-requested room details and physics T53–T55 verified
2026-10-06, starting from `35d6b3d`. Application rotation T52 verified 2026-10-05
from `cabde04`, following navigation/carrying T51; recovery begun 2026-10-03;
T44 and navigation/launcher T46–T48 verified 2026-10-04; owner-requested application objects T49 verified 2026-10-05,
starting from `4c0c078` plus the existing
uncommitted overnight work. [Decision 06](decisions/06-godot-poly-haven.md) remains
the direction. [Previous status](handoffs/15-status-before-frontend-recovery.md)
is historical; its extension/visual completion claims were contradicted by tests.

## Next task

The owner's initial graphical-application work passes bounded T65–T66: real
Chrome starts, types, clicks, scrolls, resizes, returns to its room panel and opens
multiple windows. See [handoff 37](handoffs/37-graphical-applications.md). This is
not general application compatibility. The next browser compatibility work is
`xdg_popup` menus and clipboard; both remain unimplemented, and popup requests
can still disconnect a client. The unrelated roadmap task remains below.

**P21-T45: finish targeted client close and its lifecycle acceptance.**
Recovery T36–T44 passes its bounded acceptance: furnished study, retained
compositor, live output, typing, selection/scroll and actual client resize.
See [tasks](TASKS.md) and [resize handoff](handoffs/23-godot-terminal-resize.md).
The independent panel rotation follow-up is recorded in
[handoff 29](handoffs/29-godot-application-rotation.md).
The owner's navigation and application-carrying controls are recorded in
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
`build/elsewhere --room-camera` still uses the separate legacy renderer.

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
double-click or Launch starts a new instance, including selections in recents.
Double-click an existing room panel to use that particular instance. Esc returns from search to the wheel, then to the room.
Tab still belongs to applications in application mode. Recents persist in
`.tools/launcher-state/recent.json`; tests use an isolated state directory.
Chrome launched through Applications uses native Wayland/software rendering and
a separate browser profile under `.tools/browser-profiles`; restart Elsewhere to
load the new native runtime. Browser menus using `xdg_popup` and clipboard are
not yet supported. Failed launches show a diagnostic log path under the current
launcher session directory instead of guessing which protocol is missing.
Every mapped window also has a separate live panel in the room. Single-click
selects it; double-click activates it with a quick 220ms zoom toward the camera.
Ctrl+Alt+Escape reverses the animation to its current room placement. Your
position stays unchanged. Typing starts immediately; pointer input starts when
the image settles. Left-drag moves the panel. Keep holding
left mouse to carry it while using WASD and right-mouse look; dropping it does
not interrupt held walking keys. Wheel up pushes it farther away, wheel down
brings it nearer. While both mouse buttons are held, wheel up turns the panel
left and wheel down turns it right (10° per notch), keeping its center/depth
and your view unchanged. Release right mouse to adjust depth again. Escape
restores both the original position and orientation and stops navigation.
After a move, releasing the panel lets it fall upright onto furniture or the
floor. Holding it pauses gravity; Escape restores its previous placement and
falling state. Moving the supporting chair away lets it fall again. A single
click or opening an untouched application does not drop it. Settled panels
retain their positions when returning with Ctrl+Alt+Escape, and background
applications keep updating. The chair, potted plant and bookcase use the same left-drag, wheel and
carrying controls. Walking into them pushes them across the floor; they stay
upright and come to rest. The study desk and its accessories remain fixed. Placement lasts for this session; closing
a client removes its panel. Restart restoration is not implemented.

Discovery uses freedesktop entries and icon themes without a KDE service.
Verified clients are Weston terminal and Vim through Terminal=true. Other
graphical apps may fail; Flatpak launch is explicitly unavailable for now.

Turn away from the desk to enter the hallway. The study doorway has no door.
Two room doors remain on the left, one on the right, and one at the far end.
The former first left and second right entrances are sealed. Remaining doors
swing open as you approach and stay open for the session.
The surviving middle-left doorway leads into the combined library/winter garden;
the first right doorway leads into the combined atlas/cabinet gallery. Both wings
are 7.8m wide. The inventor's room stays separate at the last left doorway, with
the grand foyer at the end. The foyer measures 20 × 24m beneath a 13m stone
vault, with a central 40-riser spiral stair leading to a gallery at 5.6m. Walk
the stair with ordinary controls, including while carrying an application.
Both levels have usable placement space; tables and plants remain movable.
Carry applications through the doorways
and release them onto tables or floors; launch new instances where you are
working. Instances elsewhere retain their locations for this session.
Rooms 01–04 are twice as deep (12.8m); room 05 retains its size. The library has
black antique bookcases with warm wood interiors
and a reading table with a green banker lamp. Common furnishings throughout
the elsewhere use the same drag/carry/rotate/push controls: bookcases, chairs,
plants, worktables, cabinets, instruments, display stands and parts bins.
Books, ornaments, table lamps and sculptures move with their furniture.
Moving a supporting table away lets an application fall and remain usable.
Fixed architecture and mounted decorations stay fixed. Restart restoration
remains unimplemented.

## Verified by automated checks in this recovery

- T67 (2026-10-09, from `9c2de0c`): project rename to elsewhere. Fresh Meson
  build and 34 tests, native/Godot binding suite, clean Godot import/runtime,
  20 study trials, real Chrome check, 8 Python tests and 62 plugin/parser tests
  pass under the new names. Maintained text and filenames have no old-name
  references; UIDs are preserved. See [handoff 38](handoffs/38-elsewhere-rename.md).

- T65–T66 (2026-10-09, from `2cdfd15`): real Google Chrome 154.0.8037.57 through
  the installed desktop entry; omnibox subsurface, local page navigation, trusted
  typing/click, wheel scroll both ways, configure/ack/commit resize with DOM reflow,
  room return, two independent panels and browser-initiated close of one window.
  Native output v1/v2/v3 and surface-tree fixtures, 34 Meson tests, standard
  binding suite, 20 graphical/headless Godot study trials, 8 Python tests and
  62 plugin/parser tests passed. Surface-tree and runtime lifecycle checks also
  passed ASan/UBSan with leak detection. Agent-inspected GPU captures and exact
  scope/commands are in [handoff 37](handoffs/37-graphical-applications.md).

- T64 (2026-10-09, from `817aff3`): foyer entry, actual 40-riser ascent carrying
  a live terminal, descent, inner/outer stair guards and gallery guard,
  upper-floor landing/typing/return, real tread support and ground-floor table
  use pass. Import/runtime, 20 Godot trials and 7 Python tests pass; five affected
  trials pass again after final lighting tuning. Four GPU views inspected and
  profiled. Occlusion, shared balusters and distance-faded local lights reduce
  hidden rendering work; detailed evidence and limits are in handoff 36.

- T63 (2026-10-09, from `95ea216`): four remaining door routes, two sealed
  entrances, twelve actual walking/live-panel crossings of removed partitions,
  continuous floors, typing, eight furniture categories and four gravity support
  regressions pass. Godot import/runtime passes; two GPU wing views inspected.
  Stable zone IDs remain separate from physical room boundaries. See handoff 35.

- T62 (2026-10-08–09, from `58d2be5`): four doubled room depths, antique library,
  eight furniture categories with drag/rotate/cancel/drop/walking-push behavior,
  scaled pot collision, carried contents, extended-room live application routes,
  moving-table support removal and subsequent shell typing pass. Import/runtime,
  18 Godot trials and 7 Python tests pass across bounded runs. The transition
  trial's client-close assertion failed once and passed unchanged on standalone
  recheck; this is recorded, not claimed resolved. Eight GPU captures inspected.
  Mesh batching preserves details/shadows and restores roughly 60 FPS median in
  the sampled room views. See handoff 34 for percentiles and limits.

- T61 (2026-10-08, from `e1deea6`): six approach-opening doors and furnished
  rooms; actual walking/carrying in/out, solid boundaries, floor/support/table
  landing, independent live terminals and shell input in two locations pass.
  Import/runtime, 17 Godot trials and 7 Python tests pass across bounded runs.
  The full script's launcher hit its 45-second limit; standalone passed in 46s.
  Agent inspected ten GPU captures; six fixed views measured median 16.66–16.68ms
  and p95 16.84–17.09ms at a 60 FPS cap. This is not a human/latency gate.
  See handoff 33. Furnishings were fixed at T61; T62 supersedes that behavior.
  Placements remain session-local.

- T60 (2026-10-08, from `139214d`): doorway traversal both ways, six closed
  doors and hallway walls, carried live terminal landing/activation, rotated
  placement bounds; five fixed GPU views inspected. Import, all 16 Godot study
  trials and 7 Python tests passed across host and isolated GPU Weston runs.
  Host launcher attempts lost focus; isolated launcher/application trials passed
  without changing product policy. See handoff 32 for exact logs and limits.

- T56–T59 (2026-10-06): chair/plant/bookcase picking, carrying, rotation, release,
  cancellation and walking pushes; real independent terminal instances and
  isolated shell state; live 220ms entry/return, endpoints, native pixels,
  interruption/focus/resize/close. Import, 15 Godot trials and 7 Python tests
  pass from `c60d037` plus T59. Walnut and transition GPU captures inspected by
  the agent; see handoff 31. Human comfort remains unobserved.

- T55 (2026-10-06): real application panels fall onto floor, desk, bookcase and
  chair, including edge contact; moving support away resumes falling. Re-grab,
  cancellation, live updates and close pass. Import, all 13 Godot study trials
  and 7 Python tests pass (host input; isolated GPU resize). Agent-inspected
  landing captures are in handoff 30; owner comfort remains unobserved.

- T54 (2026-10-05): walking pushes the chair, friction/damping stop it, and
  desk/wall/floor collision and upright constraints pass. Import and expanded
  graphical controller trial pass. See handoff 30.

- T53 (2026-10-05): fluorescent downlights, bounded subtle flicker, original
  botanical prints, clock and noticeboard. Import, rendered shadow comparison
  and client RGB test pass; fixed GPU views inspected by the agent. See handoff 30.

- T52 (2026-10-05): both buttons + wheel rotates only the held panel, preserving
  camera/center/depth and other clients. Fractional steps, switching back to
  depth, retained orientation, full-transform cancellation, blocked rotations,
  live Terminal/Vim interaction and the full study suite pass. An inspected
  1280×800 capture shows the rotated terminal and updated hint. Mouse-capture
  release no longer ends the drag early. See handoff 29; physical feel remains
  owner-unobserved for this change.

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
  process sandbox. The verified Terminal/Vim paths create owned private clients;
  Chrome uses a separate Elsewhere profile and the browser sandbox stays enabled.
  Concurrent Elsewhere frontends need separate browser profile roots.
- Client cursor images, clipboard/primary selection and popup menus. The host
  cursor remains visible; highlighting does not implement copy/paste. A right-click
  menu request can disconnect the client because xdg_popup remains unsupported.
- Original addon initialization: reproduced segmentation fault; isolated with
  `world/addons/elsewhere_godot/.gdignore`, source and binary preserved.
- Full renderer-independent compositor extraction: socket/wl_compositor/shm
  lifecycle, initial xdg-shell/seat checks and owned shm snapshots pass. Broader
  protocol conformance and popup presentation remain open; bounded subsurface
  composition and input now pass T66.
- Finished art rubric, furniture selection/cohesion, performance, physical host
  input and human comfort. No blank capture counts as visual acceptance.
- Fresh-clone dependency provenance and asset fetching/offline negative checks.

The earlier recovery preserved overnight edits and saved this local backup:
`.tools/recovery/20261003T185319Z/`. Never sweep them into a recovery commit.
