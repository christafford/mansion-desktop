# Terminal pointer and scroll — 2026-10-03

P21-T43 builds on `ddd489c` plus the preserved overnight tree. Left-drag text
selection and wheel scrollback now work in the real terminal's application view.
Enter activates it; Ctrl+Alt+Escape releases keyboard and pointer state and
returns to the room. Launch with `tools/run-godot.sh --audio-driver Dummy`.
This completes the bounded pointer gate; resize is next, P21-T44.

## Implementation boundary

Godot still uses its owned `MansionCompositorSession`; no legacy global input
seat is involved. Runtime methods are `pointer_motion(handle, x, y)`,
`pointer_button(evdev, pressed)`, `pointer_axis(horizontal, vertical)`,
`pointer_reset()`, `pointer_focus_handle()` and `pointer_grabbed()`.
Motion requires a live mapped xdg toplevel and logical surface coordinates.
Handle zero leaves only when there is no grab; reset explicitly cancels a grab.
Invalid handles/coordinates preserve the current target. Finite values are
bounded before conversion to Wayland fixed point. No resource pointers cross
the binding and no pointer state is a persistent artifact identity.

The seat filters events to pointer resources of the focused client. Button
transitions are deduplicated and bounded to the eight Linux mouse button codes.
An implicit grab pins motion/releases to the originating surface until all held
buttons are released. Another target cannot steal it. Reset sends releases
before leave; destruction clears references without a leave naming a dead
surface. A destroy listener handles disconnect; the runtime also revokes focus
on detach/role loss after dispatch. Late pointer binding gets current enter and
coordinates. Serial/time use the owning display and monotonic milliseconds.
Seat version 4 sends axis events, without unsupported version-5 frame/axis-source
or discrete-scroll events.

`pointer_map.gd` maps the TextureRect's actual aspect-fitted drawn rectangle to
committed logical dimensions retained by `terminal_screen.gd`. The texture is
already inverse-transformed upright, so pointer coordinates must not rotate a
second time. Buffer scale is accounted for by the logical dimensions. Native
and shrunk views share this mapping, including fractional internal letterboxing.
World-mode and margin clicks are not delivered. During a drag, outside motion
and release still reach the client; new outside presses and scroll do not.
Left/right/middle/back/forward map explicitly to Linux button codes. Wheel factors
produce signed axes; pan gestures also have a delivery path, but physical
trackpad behavior has not been observed. Host focus loss and mode exit reset
pointer state before restoring world controls.

Protocol references checked against installed `wayland.xml`:
[Wayland input model](https://wayland.freedesktop.org/docs/book/Protocol.html) and
[pointer specification](https://wayland.freedesktop.org/docs/html/apa.html).
Real selection/scroll behavior was checked against
[Weston 15.0.1 terminal source](https://gitlab.freedesktop.org/wayland/weston/-/blob/15.0.1/clients/terminal.c).

## Verification

Observer: Codex agent, 2026-10-03, T43 working tree based on `ddd489c`, committed
with this handoff. Godot `4.7.2.stable.official.ed1daf0bf`, godot-cpp
`507ed9d840c01a3c5b2a39af8bb4000bfac30bf5`; Compatibility/OpenGL 4.6,
Mesa 26.2.4, AMD Custom GPU 0405, 1280×800. Installed Weston 15.0.1-3,
monospace size 16, `/bin/sh` (Bash). No host configuration changed.

```sh
meson compile -C build
meson test -C build --print-errorlogs
tools/check-godot-binding.sh
tools/check-godot-study.sh
```

- C++ build and 33/33 Meson tests pass, including legacy input regressions.
- Standard binding, protocol lifecycle, owned frames and keyboard tests pass.
  `runtime-pointer` with `pointer-client` adds two-client isolation, same-client
  switching, exact fractional coordinates/axis values, Linux buttons, duplicate
  transitions, multiple held buttons, outside drag, denied grab stealing,
  explicit reset, late binding, detach, surface destruction, abrupt disconnect,
  invalid values/handles and restart rejection of stale handles.
- Pointer runtime passes AddressSanitizer, UBSan and enabled leak detection.
  This instruments the native project runtime/seat, not Godot itself. Reproduce
  using handoff 21's sanitizer recipe with `tests/test_runtime_pointer.cpp`,
  output `runtime-pointer-asan`, fixture `build-godot-probe/pointer-client`.
- Godot import/runtime, eight image transforms, 27 keyboard mappings, rendered
  study/controller, eleven GPU color patches and two real output sessions pass.
- 302 pointer mapping assertions pass: eight transforms, scales 1/2, native and
  shrunk views, corners, exclusive edges, internal/external margins, outside
  grabs, invalid input, button mapping and signed/fractional wheel steps.
- The real keyboard test still passes (eight repeated characters and 7,571
  matching opaque source/capture samples in this run).
- `terminal_pointer.gd` uses production Godot input routing. It selects a real
  output line, verifies the selection stops changing after release, releases
  outside the image, scrolls `seq 1 100` back thirty rows and restores it, exits
  during a drag, resumes/stops world movement, checks camera/pointer release,
  injects host-focus loss and disconnects the owned client while dragging.
  Selection changes 7,496 pixels in the asserted text region; scroll changes
  1,877. These support the test assertions, not a general usability claim.
- [Rendered evidence](../evidence/terminal-pointer/) was inspected: correct text
  highlight, rows 77–100 versus 47–71, and live monitor after return to the room.

The first selection test used coordinates in Weston's invisible 32px shadow
margin, not its text. Inspecting the actual source frame and protocol geometry
identified this test error; the fixture coordinates now include those margins.
Several early GUI trials also lost focus during startup and correctly failed.
The test now focuses its own window, settles and asserts focus before injecting
input, with explicit activation/grab preconditions. It does not bypass production
focus policy. Keep graphical tests focused and avoid concurrent desktop tests.
The final complete study run passes with error-free success markers. Benign
Weston missing `dnd-copy`/`dnd-none` cursor warnings remain.

Logs: `.tools/recovery/t43/`, plus native binding logs under `build-godot-probe/`
and detailed study logs in `/tmp/mansion-study-check.3RIMAC/`. Test captures are
regenerated in `.tools/terminal-pointer-test/`, not over historical evidence.

## Limits and exact continuation

Only the top-level image is presented/hit-tested. Client `set_cursor` images are
still ignored; the visible pointer is the host cursor. Text highlighting works,
but no data-device clipboard, primary selection or drag-and-drop transfer is
implemented. Copy/paste is not accepted by this task. Popup menus are also
unsupported: `xdg_surface_get_popup` still posts a fatal protocol error, so a
right-click menu request can disconnect a client. Button delivery itself is
verified by fixtures; that is not popup-menu compatibility. Do not advertise
these features or count T43 as the full terminal usability gate.

Next **P21-T44** implements bounded xdg configure/ack/commit resize, respecting
client window geometry and its committed size. Weston includes a 32px shadow
outside geometry (currently `(32,32,803,593)` in an `867×657` buffer); do not
confuse window geometry, buffer size and logical surface coordinates. Recompute
presentation and pointer mapping together from committed state and keep old
pixels valid while waiting. Follow the fixture and real-client acceptance in
TASKS.md. T45 then adds compositor close and explicit relaunch; its integration
review must carry forward cursor, clipboard and popup limitations into concrete
follow-up tasks. No broader or human gate is ticked. Physical input, latency,
final art and full desktop compatibility remain unverified. Unrelated overnight
source/binary changes remain outside this commit.
