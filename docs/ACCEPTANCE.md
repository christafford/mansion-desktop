# Real-client and visual acceptance

Status: procedures unless explicitly recorded as evidence. Synthetic tests and
screenshots are supporting evidence, not usability acceptance. Human tasks in TASKS.md remain unchecked until a person records them.

## 2026-10-02 audit boundary

The owner confirmed legacy room navigation and the upside-down export fix in
this conversation after `99fcedb` / `3dfaa26`. This is not a terminal, Godot or
furnished-room acceptance trial. No human task is ticked from that feedback.
The audit ran isolated Godot CLI import/runtime checks and inspected source/cache
contents, not a Godot visual or live-client trial. See
[the audit handoff](handoffs/11-godot-reality-audit.md) for exact scope/results.
P21-T00/T01/T02/T03/T10 are reopened. A `.so` build, cache size, or legacy test
pass cannot satisfy their revised explicit checks in [TASKS.md](TASKS.md).

## 2026-10-05 agent-run application rotation (P21-T52)

Based on `cabde04` plus T52, Codex verified both-buttons + wheel yaw through
injected events on a real Weston Terminal, alongside Vim. Camera, panel center,
depth and the other client stay unchanged. Both directions/fractional steps,
return to depth adjustment, orientation retained after drop/application return,
full-transform cancellation and rejected obstructed rotations pass. The full
study suite passes with the host input seat and isolated GPU resize.
[Rendered rotation](evidence/application-rotation/rotating-terminal.png) inspected
at 1280×800, Godot 4.7.2 Compatibility/OpenGL, Mesa 26.2.4, AMD Custom GPU 0405:
terminal visibly angled, Vim unchanged, and contextual rotation hint visible.
See [handoff 29](handoffs/29-godot-application-rotation.md) for commands and logs.
Physical feel is not observed; no human gate is ticked.

## 2026-10-05 agent-run navigation and carrying (P21-T51)

Based on `c789f37` plus T51, Codex verified the new controls with rendered Godot
input tests and real Weston Terminal/Vim clients: WASD always strafes/walks,
mouse-look remains where aimed after release, Shift speeds up, Ctrl/M slows,
and walking/turning continues while holding a live application. Dropping does
not stop held movement; wheel up pushes away and down pulls closer. Existing
collision, cancellation, typing, selection, scrolling and resize checks pass.
The full study suite passes with host input and isolated GPU-rendered resize;
see [commands and limitations](handoffs/28-godot-natural-navigation.md).

[Rendered controls and live panels](evidence/natural-navigation/controls-and-panels.png)
were inspected by Codex at 1280×800, Godot 4.7.2 Compatibility/OpenGL,
Mesa 26.2.4, AMD Custom GPU 0405. This image shows the new control hints and
real Terminal/Vim panels after placement; movement claims come from the event
trials, not this still image. Physical keyboard/mouse feel is not observed.
Human tasks remain unchecked. The owner trial is described in handoff 28.

## 2026-10-05 agent-run application objects (P21-T49)

Based on `88f9938` plus T49 and pre-existing uncommitted work, Codex exercised
real Weston Terminal and Vim simultaneously using injected Godot input. Separate
live previews keep updating; dragging one preserves the other's placement;
double-clicking the terminal then typing writes the expected `TARGET` file.
Room return preserves identity/placement, and real client exit during drag clears
only that client's object. Cancellation, occlusion, bounds and owned shutdown
also pass. [Rendered host captures](evidence/application-objects/) show both
applications in the furnished room before/after movement. Observer: Codex agent,
Godot 4.7.2 Compatibility/OpenGL, Mesa 26.2.4, AMD Custom GPU 0405, 800×800 window
on the current portrait host. No physical-input or human comfort trial is claimed.

Use `tools/run-godot.sh --audio-driver Dummy`, launch an application from Tab,
then Ctrl+Alt+Escape to return. Double-click its panel to resume; left-drag to
move, wheel during drag for distance, Escape to cancel. Place Terminal and Vim,
move each, resume each and confirm ordinary typing/selection. Restart placement
restoration and general application compatibility are not implemented by T49.
See [handoff 26](handoffs/26-application-objects.md) for fresh regression results,
the pre-existing navigation failure and isolated four-size resize evidence.

## Current Project 21 acceptance

Use Decision 06, ART-DIRECTION.md and GODOT-INTEGRATION.md for the new frontend.
The commands below describe the supplied legacy executable and are historical
procedures for it, not Godot launch commands. P21-T00/T01/T11 must document the
new actual commands in TOOLCHAIN.md and this file after implementing them.

P21-T48 launcher acceptance (Codex agent, 2026-10-04, based on `7316f78`): real
installed Vim launched from the radial menu's search dialog, received typing
inside Weston, and returned to the existing terminal through a recent-app click.
The original terminal exited and relaunched; a shell-written file confirmed
typing in the replacement. The graphical test verifies modal/camera isolation,
ordinary application Tab, eight-slot layout, persistence and cleanup. See
[labeled rendered captures](evidence/application-launcher/) and
[handoff 25](handoffs/25-application-launcher.md). Eight populated history slots
are a layout fixture, not eight app-compatibility passes. General graphical-app
support, client close requests, physical input and human comfort remain open.

For P21-T09/P21-T19/P21-T28, keep fixed rendered world/application views with
revision, camera, renderer, resolution, machine and observed defects. Inspect
the images; never infer visual quality from a successful headless import.
For P21-T18/T19, run a real native software terminal on the private socket and
record typing/output, modifiers, pointer/scroll, resize, repeated mode switches,
host-focus loss, close/relaunch and responsive world movement. Record moving
content and input results, not just a static screenshot. Include exact client
and environment; fixture and CPU-copy proofs do not establish GPU compatibility.

P21-T33 audits the implemented workspace and prepares the owner trial. P21-T34
requires a person. Agent GUI observation may satisfy an explicitly agent-observed
task but never ticks that human task. Performance on another machine is provisional
until measured on the target Steam Deck. Missing GUI or target hardware is a
blocker for its dependent acceptance, not permission to invent results; complete
other independent eligible tasks first.

Record: task IDs, revision/date, command, environment, expected/actual result,
evidence path, observer, pass/fail and limitations. Track material/lighting,
interaction, real-client compatibility and performance separately. A correct
test suite does not erase a failed visual criterion.

## 2026-10-03 agent-observed Godot live output (P21-T41)

Codex inspected the real Weston terminal 15.0.1 output in the Godot study:
upright text, changing clock/counter, preserved aspect ratio and clearing on
disconnect. [Room and close-up captures](evidence/live-terminal/) were rendered
at 1280×800 using Godot 4.7.2 Compatibility, Mesa 26.2.4, AMD Custom GPU 0405.
Source: T41 implementation committed with [handoff 20](handoffs/20-godot-live-terminal.md),
based on `3a8843f`. Reproduce with `tools/check-godot-study.sh`.

The real terminal check exercises two sessions and asserts owned process/socket
cleanup. Separate synthetic GPU color patches and injected controller events
pass. `client-frame.png` is a source frame, not a rendered screenshot. No actual
terminal typing, pointer interaction, application mode or physical-input trial
is claimed. Human acceptance remains **not observed**; T42–T45 implement the
remaining interaction before the broader terminal usability procedure.

## 2026-10-03 agent-run Godot keyboard interaction (P21-T42)

Codex injected Godot keyboard events through the production application-mode
path into the real Weston 15.0.1 terminal and `/bin/sh`. Shell-written output
files verify mixed-case/punctuation typing and Backspace; additional commands
verify repeat and Ctrl+C interruption. Enter activates the same live window,
Ctrl+Alt+Escape returns to world, movement resumes, host-focus-loss notification
clears focus, and typing `exit` while Shift is held restores world mode.

[Application and post-exit captures](evidence/terminal-input/) were inspected at
1280×800 using Godot 4.7.2 Compatibility, Mesa 26.2.4, AMD Custom GPU 0405.
The flat view preserves native pixel size and 7,571 sampled client pixels.
Revision: T42 implementation committed with [handoff 21](handoffs/21-godot-terminal-keyboard.md),
based on `d7a590c`. Reproduce with `tools/check-godot-study.sh`.

This passes the bounded keyboard gate with a physical US mapping. The events
were injected; physical host input, IME/layout selection, pointer/scroll, resize,
compositor close/relaunch, latency and human usability remain **not observed or
unimplemented**, as applicable. No human task is ticked.

## 2026-10-03 agent-run Godot pointer interaction (P21-T43)

Codex injected Godot mouse events into real Weston 15.0.1: a left drag highlights
the printed output line, release stops the selection, and a second drag releases
outside the image. Three wheel-up events reveal rows 47–71 after `seq 1 100`;
wheel-down restores rows 77–100 and the prompt. Mode exit during a drag releases
input; room movement resumes/stops and the camera stays released. Host-focus-loss
notification and abrupt owned-client disconnect while held clear pointer state.

[GPU captures](evidence/terminal-pointer/) were inspected using Godot 4.7.2
Compatibility, Mesa 26.2.4, AMD Custom GPU 0405, 1280×800. Revision: T43 changes
committed with [handoff 22](handoffs/22-godot-terminal-pointer.md), based on
`ddd489c`. Reproduce with `tools/check-godot-study.sh`. Native protocol and mapping
tests separately cover client isolation, coordinates, button/axis codes and
scale/transform/letterboxing; they do not substitute for this real-client trial.

This passes selection/scroll, not clipboard transfer. Client cursor images and
popup menus remain unsupported (a popup request can disconnect a client).
Physical mouse/touchpad input, latency, actual client resize, compositor close/
relaunch and human usability are not accepted by this evidence. No human task
is ticked.

## 2026-10-04 agent-run Godot client resize (P21-T44)

Codex changed the real host window size four times after terminal activation:
1000×700 → 1200×780 → 900×620 → 1280×800. Weston committed new buffers and its
shell reported 77×22 → 95×25 → 68×18 → 103×26 columns/rows. Typing and selection
worked at every size. 8,573 sampled opaque pixels matched native-size GPU views;
return to world showed the updated live monitor. This proves client reflow in
the tested sizes, not merely scaling an old texture.

[Five GPU captures](evidence/terminal-resize/) were inspected with Godot 4.7.2
Compatibility, Mesa 26.2.4, AMD Custom GPU 0405, at the listed viewport sizes.
Client: installed Weston 15.0.1-3, monospace 16, `/bin/sh` (Bash). Revision: T44
changes committed with [handoff 23](handoffs/23-godot-terminal-resize.md), based on
`2e304a9`. Reproduce with `tools/check-godot-study.sh`. Native fixture/sanitizer
checks separately cover delayed/declined choices, pending/committed metadata,
limits, serials, retained frames, bounded queues and destruction while pending.

Input and host-size changes were injected by the agent. Physical mouse-driven
host resize, HiDPI host behavior, very small windows, latency and human comfort
remain unobserved. Compositor close/relaunch, clipboard, client cursors and popup
menus remain unfinished. No human task is ticked.

## Legacy baseline and procedures

Run from the repository root in the normal development container/session:

```sh
./check-deps.sh
meson setup build --buildtype=debug  # first time only
meson compile -C build
meson test -C build --print-errorlogs
./build/mansion-desktop --help
git rev-parse HEAD
```

Run sanitizer checks separately as documented in TASKS.md. Record versions of
Meson, Wayland/protocols, Mesa/driver, GPU and the chosen terminal. Install missing
packages personally using GETTING_STARTED.md; an autonomous session records the
blocker. Do not change host services, global input permissions, or login sessions.

## Terminal trial: P1-T10, P4-T04 and recovery P4-T17

Current source presents through a nested Wayland client window and needs the
host WAYLAND_DISPLAY plus weston-terminal. The owner confirmed basic world
navigation and upright output on 2026-10-02, but this terminal/input trial has
not been observed. These remain legacy procedures, not the Project 21 work order.

```sh
./build/mansion-desktop --flat --launch weston-terminal
./build/mansion-desktop --room-camera --launch weston-terminal
```

Perform the flat trial first, then the room trial. Mansion prints its private
MANSION_SOCKET. Ensure the terminal connects there rather than appearing as an
ordinary host window. Do not override the host WAYLAND_DISPLAY globally; the
launcher sets child environment. If launching by hand, use its private display
name with the same XDG_RUNTIME_DIR and explicit native toolkit backend.

1. Confirm terminal content appears and updates; record a screenshot including
   the Mansion window. Type a short command and confirm visible output.
2. In room mode, walk or use the documented T teleport, target the monitor and
   Enter to application mode. Confirm the full-size terminal is readable.
3. Type lowercase/uppercase, punctuation and modifier combinations; select text
   with the pointer and scroll output. Test corners/center and any letterboxing.
4. Resize the host window smaller and larger. Verify updated text/pointer mapping
   and repeated configure behavior. Record clipping or scaling issues.
5. Use F12 (default reserved world key) to return to the world and re-enter at
   least five times. Confirm the same application/session/output persists.
6. Hold a key or pointer button while changing modes/focus, then release it;
   verify no stuck input. Switch to another host app and type; no Mansion client
   should receive that input. Record any host shortcuts Mansion cannot capture.
7. Close the terminal from its UI and confirm Mansion stays responsive in world
   mode. Restart with a fresh launch and repeat; client close must not leave stale
   focus/content masquerading as a live window.
8. Quit Mansion using its host close control. Confirm the host desktop remains
   usable and launcher-owned child shutdown behavior matches documentation.

Record observed pass/fail per step. If flat and room evidence covers the earlier
human tasks in full, a person may mark P1-T10/P4-T04 and P4-T17 from the same
record; partial evidence does not pass the combined procedure.

## Furnished-room trial: P4-T27

After P4-T20–P4-T26, record the exact scene/asset revision and repeat the room
terminal trial. Inspect textures, furniture scale and lighting. The application
must stay unlit/readable in world and application modes. Target and move the
supported object between slots; open/close the door and cross the doorway without
being trapped. Test reduced-motion and keyboard access. Record frame-time/copy
metrics with a fixed workload and personally describe comfort/legibility; pixel
checks cannot supply those observations. Restart persistence is a later P6 check.

## Multi-application trial: P5-T10

P5-T08 must supply real native terminal/browser/editor versions and launch
commands using disposable test profiles/data. Launch all three; switch by the
documented keyboard and object controls, type/click/scroll in each, open menus
and a dialog, and verify related surfaces stay attached to the correct parent.
Request close with an unsaved document and decline it; the app must remain usable.
Close one of two windows in one application process, then another application;
remaining windows continue working. Assign an ambiguous window manually.
Record exact unsupported protocols/buffer paths; no universal compatibility claim.

## Evidence record template

Copy one record per trial into the applicable handoff; summarize/link it from
STATUS. Keep screenshots/logs at named repository-relative evidence paths only
when suitable to commit; avoid personal terminal history or private app data.

```text
Task/gate:
Observer and type (human / agent GUI / automated):
Date, timezone, source revision, dirty-tree differences:
Host OS/session, container, backend, GPU/driver, EGL/GLES versions:
Application/version, scene/assets, configuration:
Exact build/run/check commands:
Step-by-step expected and observed results (pass/fail/not observed):
Screenshots and diagnostic log paths:
Normal/sanitizer results and measured workload/performance:
Known limitations, blocker and smallest next action:
Gate outcome and exact next scope authorized:
```

An agent may collect separately labeled GUI evidence where its tools permit;
tasks explicitly marked (human) still require a person's observation. Leave
unperformed steps "not observed" and product gates pending. A test exit code or
successful mapping log does not prove text readability or interactive comfort.

## Owner-requested room details and physical placement: T53–T55

2026-10-05–06 automated and agent-rendered evidence is recorded in
[handoff 30](handoffs/30-study-details-and-physics.md). This does not tick a human
gate. Personal comfort/appearance observation for these changes: **not observed**.

Run `tools/run-godot.sh --audio-driver Dummy`. Look up at both twin-tube fittings,
inspect the soft brightness variation and moving shadows below them, then view
the botanical prints, clock and noticeboard. Walk into the chair from the back
and side; it should yield and stop against furniture/walls without tipping.
Drag the live terminal over the floor, desk, chair seat and bookcase top; release
it and check that its bottom stops at the first surface it meets. Lift it again,
rotate with both mouse buttons plus wheel, and cancel with Escape. Move the chair
out from underneath a supported panel. Confirm the panel falls again, stays
live and still accepts typing after double-click activation. Record comfort,
readability, visual defects and measured performance separately from automated
collision/input evidence. Session placements are not restored after restarting.

## Owner-requested furnishings and application transitions: T56–T59

2026-10-06 agent-run evidence, commands and rendered captures are recorded in
[handoff 31](handoffs/31-furnishings-and-transitions.md), starting from `e618487`.
Human comfort/physical input observation remains **not observed**.

In the visible study, left-drag the chair, plant and bookcase, carry them with
WASD, change distance with the wheel and rotate with both buttons plus wheel.
Release to drop them; Escape restores the original placement. Walk into each
and inspect floor/wall/furniture contact. Check walnut grain and panel joints
around the window and decorations. Launch Terminal twice through the wheel;
type different commands, return to the room, reopen each panel and close one.
The other should retain its shell state. Double-click a room panel to see its
live surface approach the camera; Ctrl+Alt+Escape sends it back. Try reversing
quickly, resizing, losing host focus and closing the client. Confirm comfortable
motion and readable native text separately from the agent's geometry, pixel
and input assertions. Session placements still reset when Mansion restarts.

## Owner-requested hallway: T60

2026-10-08 agent-rendered and injected-controller evidence from `139214d` plus
T60 is in [handoff 32](handoffs/32-oil-lit-hallway.md). Human physical-input,
lighting preference and motion comfort observation remains **not observed**.

In the study, turn away from the desk and walk through the open framed doorway.
Count three closed doors on the left, two on the right and one at the far end.
Inspect the burgundy carpet, oil reservoirs/chimneys/flames and warm wall light.
Walk the full hall and return; closed leaves and walls should stop you. Carry
a live application through the doorway, drop it on the hallway floor, activate
it and return to the same position. Movable furnishings use the same placement
volumes. Doors have no opening interaction in this request.
