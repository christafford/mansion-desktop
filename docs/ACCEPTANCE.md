# Real-client and visual acceptance

Status: procedures only; no check below has been performed by this documentation
update. Synthetic tests and screenshots are supporting evidence, not usability
acceptance. Human tasks in TASKS.md remain unchecked until a person records them.

## Current Project 21 acceptance

Use Decision 06, ART-DIRECTION.md and GODOT-INTEGRATION.md for the new frontend.
The commands below describe the supplied legacy executable and are historical
procedures for it, not Godot launch commands. P21-T00/T01/T11 must document the
new actual commands in TOOLCHAIN.md and this file after implementing them.

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
host WAYLAND_DISPLAY plus weston-terminal; the X11 host window was removed on
2026-09-28 and the Wayland window is unverified (reported flicker, no input).
P4-T12/P4-T16 must record the commands that actually work before this trial.

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
