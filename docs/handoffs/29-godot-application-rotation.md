# Rotate held applications — 2026-10-05

P21-T52, owner's bounded follow-up to T51, based on `cabde04`.

## Behavior

Hold left mouse on an application and hold right mouse to look. Wheel up turns
only that panel left, wheel down turns it right, at 10 degrees per notch.
Fractional wheel steps give proportionally smaller turns. Rotation is upright
around the panel's center; it does not change camera orientation or drag depth.
Release right mouse to resume the existing wheel depth control. Walking and
mouse-look still work while carrying; drop retains the new orientation.

The manager tentatively changes yaw, reuses the existing placement bounds and
collision checks, and rejects an obstructed orientation without moving the center.
It now saves the entire original transform, so Escape, launcher entry and focus
loss restore both position and orientation. Application-mode input is unchanged.
The visible hint explains rotation and switches to left/right directions while
mouse-look is captured during a drag.

A regression exposed a related drag-lifetime problem: switching from captured
to visible mouse mode emits `mouse_exited` on this host even while left remains
held. The old handler committed the drag at that point. Drag ownership now ends
on left-button release or cancellation/focus loss; a pointer-exit notification
alone does not drop a held application. GUI controls still cannot swallow its
release. Temporary tracing used to establish that callback was removed.

## Verification

Codex agent, Godot `4.7.2.stable.official.ed1daf0bf`, Compatibility/OpenGL 4.6,
Mesa 26.2.4, AMD Custom GPU 0405, Weston Terminal 15.0.1 and installed Vim.

- `SDL_JOYSTICK_LINUX_CLASSIC=1 tools/validate-godot-project.sh`: native extension
  build plus Godot import/runtime pass; `.tools/t52/import-fixed.log`.
- `SDL_JOYSTICK_LINUX_CLASSIC=1 GODOT_BINARY="$PWD/.tools/t51/godot-check.sh" tools/check-godot-study.sh`: exit 0, all 11 Godot trials and seven Python tests
  pass. Uses the [T51 host-input/isolated-resize setup](28-godot-natural-navigation.md).
  Full output: `.tools/t52/suite.log`; individual logs:
  `/tmp/elsewhere-study-check.H4k3iH/`. No error markers or failed assertions.
- `APPLICATION_OBJECTS_OK clients=2 failures=0`; native-pixel typing/selection,
  scrolling, four-size resize, launcher, walking/carrying, GUI releases and owned
  client shutdown remain covered. No C++ source changed; no fresh Meson or
  sanitizer claim. Existing optional cursor/icon/decoration and Weston Vim
  escape-sequence warnings remain.
- [Rendered rotation capture](../evidence/application-rotation/rotating-terminal.png)
  inspected at 1280×800: the held live terminal is turned 30° from its initial
  orientation, Vim remains in place, and the three-line control hint fits.

Tests inject Godot events into real Weston Terminal/Vim interactions. The panel
trial covers both rotation directions, fractional/repeated steps, wheel-release
suppression, unchanged center/depth/camera/other panel, return to depth adjustment,
retained orientation through drop and application return, and full-transform
cancellation. Placement fixtures separately reject rotation through room bounds
and another panel. Existing walking/look/carry and client-input tests remain.

Physical mouse-wheel feel is not inferred from injected events. This does not
complete human gates, durable placement or general application compatibility.
P21-T45 remains the next roadmap task; no broader run is authorized here.
