# Familiar navigation and carrying applications — 2026-10-05

P21-T51 is the owner's bounded control redesign, including their follow-up request
for walking while dragging applications. Based on `c789f37`; no broader roadmap
run. It supersedes T46/T47's historical control choices, not their dated evidence.

## Controls and implementation

- W/S forward/back, A/D strafe, relative to the current heading at all times.
- Hold right mouse to look; right turns right and up looks up. Release to free
  the pointer without changing yaw or pitch. No return animation, camera bob,
  acceleration delay or momentum after releasing movement keys.
- Walk at 2.4 m/s, hold Shift for 4 m/s, hold Ctrl or toggle M for 1 m/s.
  Slow mode wins over Shift. Diagonal speed equals cardinal speed, and looking
  up/down does not change walking height or horizontal speed.
- Escape stops navigation/releases capture; Home also restores the arrival view.
  Host focus loss clears movement/capture and rejects input until focus returns.
- Left-drag an application, then keep holding left mouse while walking with WASD
  or looking with right mouse. The grabbed panel follows the camera even when
  the pointer is stationary. Dropping it preserves held movement/look input.
- Wheel up pushes a held application farther away; down pulls it nearer.
  Fractional wheel factors preserve smaller steps. Existing depth limits, room
  bounds and rejected furniture/player/panel overlaps still apply.
- Escape restores the panel's pre-drag position and stops navigation. Launcher
  entry, application activation and host focus loss cancel the drag. Ordinary
  application typing, selection and scrolling use the existing application mode.

`game_world.gd` now uses one movement mapping. Mouse-look directly adjusts yaw
and bounded pitch; releasing it only changes pointer ownership. Owned key/button
releases are handled before GUI controls can swallow them. The compositor and
C++ input implementation are unchanged.

`application_objects.gd` no longer sets application mode to freeze the player
while dragging. Its physics update follows a saved screen-space grab and a
camera-local offset through walking and turning. Only drag-specific inputs are
consumed; navigation passes to the player. The manager no longer writes the
application/launcher mode flag on drop, avoiding conflicting ownership.

## Verification and limits

Codex agent, 2026-10-05, Godot `4.7.2.stable.official.ed1daf0bf`, Compatibility /
OpenGL 4.6, Mesa 26.2.4, AMD Custom GPU 0405, Weston Terminal 15.0.1 and Vim.
No C++ source changed; the runtime extension build and Godot import/runtime
validator pass. No new Meson/sanitizer result is claimed.

- Graphical controller: 56 textured surfaces, zero failures. Tests cover all
  four movement axes with/without look, diagonal/opposing keys, speeds and
  precedence, walking under rotated yaw/extreme pitch, stop/collision behavior,
  repeated grabs without camera return, pitch limits, GUI releases, focus
  loss/reentry, Escape/Home and application-mode isolation. A final rerun with
  a focused LineEdit swallowing GUI input also passes; log:
  `.tools/t51/controller-final.log`.
- Real two-client trial: view-relative carrying under walking and turning,
  input isolation, held navigation after drop, stationary dropped placement,
  both wheel directions and half-step magnitude, existing two-window typing,
  backgrounds, independent identity, cancel/bounds/occlusion and shutdown pass.
  `APPLICATION_OBJECTS_OK clients=2 failures=0`.
- Full `tools/check-godot-study.sh` passes: 8 transform, 27 keyboard-map and
  302 pointer-map cases; controller; GPU color; live output; keyboard; selection /
  scroll; four-size resize; launcher; application objects; six desktop-helper
  tests and one launcher-script test. Summary log: `.tools/t51/suite-host.log`;
  individual logs: `/tmp/mansion-study-check.DNIm8p/`. Documentation links and
  the new numeric task ID validate; pre-existing legacy P1-T06 duplicates remain
  unchanged. No debug tracing remains.
- Four resize grids: 77×22, 95×25, 68×18, 103×26; 8,573 native-pixel samples.
  Host terminal-input test: 11,163 native-pixel samples.
- [1280×800 rendered capture](../evidence/natural-navigation/controls-and-panels.png)
  inspected: updated navigation/carry/wheel hints fit without clipping and both
  real client panels remain visible and upright after placement. Detailed work
  still uses application mode. This is not final room-art acceptance.

The full suite ran with `SDL_JOYSTICK_LINUX_CLASSIC=1` and a local `GODOT_BINARY`
wrapper, `.tools/t51/godot-check.sh`. The wrapper runs the pinned engine on the
host for input/capture, and only dispatches `terminal_resize.gd` to:

```sh
weston --backend=headless --fake-seat --renderer=gl \
  --width=1600 --height=1000 --socket=mansion-t51-resize --no-config \
  --idle-time=0 --log="$PWD/.tools/t51/weston-resize.log" -- \
  /usr/bin/env WAYLAND_DISPLAY=mansion-t51-resize \
  tools/Godot_v4.7.2-stable_linux.x86_64 --display-driver wayland \
  --path world --audio-driver Dummy --max-fps 60 \
  --script res://tests/terminal_resize.gd
```

The normal host resize limit previously recorded in handoff 26 is avoided
without changing the desktop or weakening assertions. Running the entire suite
on a headless Weston output failed: without `--fake-seat`, capture reports
`Parameter "ss" is null`; with it, the pointer trial cannot establish host
focus. These are not passing input trials. Final input tests use the actual host
seat. Logs remain in `.tools/t51/suite.log` and `suite-seat.log`.

The initial controller boundary assertion needed an approximate comparison for
Godot's float pitch clamp; the clamped values satisfy the intended ±1.35 radians.
The carry test initially checked drop before Godot dispatched an event injected
from `physics_frame`; its mouse helper now waits through the next input dispatch.
Temporary tracing was removed. Existing optional icon/cursor/Wayland decoration
and Weston Vim escape-sequence warnings remain; no application-compatibility
expansion is claimed.

Tests use injected events and real Weston Terminal/Vim clients. They do not
establish physical mouse feel or human comfort. The owner should try walking
around furniture while looking, releasing/re-grabbing look, slow/fast movement,
carrying a panel while turning, both wheel directions, dropping while still
walking, Escape cancellation, and ordinary terminal typing after activation.

P21-T45 remains next: targeted xdg close and lifecycle acceptance. Broader art,
physical-input, persistence, popup/cursor/clipboard and product gates remain open.
