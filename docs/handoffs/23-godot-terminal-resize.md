# Real terminal resize — 2026-10-04

P21-T44 builds on `2e304a9` and the preserved overnight edits. Resizing the Godot
host window in application mode now sends a real xdg size suggestion. The
terminal commits new pixels and changes its character grid. Typing, pointer
selection and the world monitor continue using committed content. Next: P21-T45,
compositor close requests and explicit relaunch.

## Run and behavior

```sh
tools/run-godot.sh --audio-driver Dummy
```

Press Enter to activate the terminal, then resize the host window. Ctrl+Alt+Escape
returns to the room. The current terminal size remains on the monitor; returning
to application mode at the same size does not repeatedly configure a client that
rounded or declined the previous request. Type `exit` to close the shell;
relaunch still requires restarting the study until T45.

The viewport no longer stretches a fixed 1280×800 canvas. UI coordinates follow
the actual host viewport, allowing both terminal reflow and native-size text.
Application mode debounces size changes for 120ms, subtracts UI space and the
committed surface margins outside window geometry, and requests at most 2048 in
either dimension. A further 16px per side accommodates client size increments.
The first rendered test caught fractional text scaling because Weston rounded
some suggestions upward to whole character cells. That extra space resolves it
for all four tested sizes. Clients can choose another size; oversized or
unresponsive clients keep their old pixels with the existing aspect-fit fallback.
No retry loop assumes a client's chosen size must equal our suggestion.

## Protocol and ownership

`request_resize(handle, width, height)` on the owned runtime/standard binding
requires a live mapped toplevel and dimensions in 1..2048. It applies committed
min/max limits; a resulting minimum beyond the bound is rejected. Equal size
suggestions are deduplicated. A 64-entry pending-configure bound rejects further
requests without discarding valid serials. The frontend logs a warning and keeps
current content on rejection; a later viewport change can request again.
No client is killed or forced to resize. The legacy global seat is not involved.

`window_state(handle)` returns value metadata, separate from owned frame bytes:
committed logical geometry and min/max limits, last suggested dimensions, and
sent/acked/committed configure serials. Empty means no live mapped target.
These IDs remain runtime protocol state, never persistent world identity.
A configure changes only the suggestion/serial fields. An acknowledgement
consumes that serial and older pending ones but does not replace pixels or apply
geometry. The next successful surface commit applies geometry/limits and records
the last acknowledged serial. Clients may delay, coalesce acknowledgements,
choose a different size or commit unchanged pixels after declining.

`set_window_geometry` and toplevel min/max sizes are now double-buffered.
Negative limits and min/max conflicts are protocol errors. Explicit geometry is
clamped with wide arithmetic to the committed root surface and stays fixed until
set again; unset geometry follows the root surface bounds. Empty intersections
are rejected. Subsurface trees remain unsupported. Geometry-only commits do not
copy pixels or advance the owned frame revision; callers query window metadata
separately after the same main-thread pump. Pointer mapping continues to use the
upright image and committed logical surface extent, including scale/transform;
it must not use the requested window-geometry dimensions.

Both frontend commit paths call `xdg_shell_on_surface_applied` after their surface
state is applied. This is the only change to the already-dirty compositor source
files. Their earlier edits were backed up in `.tools/recovery/t44/` and excluded
from the commit. Shared xdg/seat ownership and old headless/nested paths remain.

Protocol semantics were checked against the installed official
`wayland-protocols/stable/xdg-shell/xdg-shell.xml`; upstream
[xdg-shell source](https://gitlab.freedesktop.org/wayland/wayland-protocols/-/blob/main/stable/xdg-shell/xdg-shell.xml).

## Verification

Observer: Codex agent, 2026-10-04. T44 changes based on `2e304a9`, committed with
this handoff. Godot `4.7.2.stable.official.ed1daf0bf`, godot-cpp
`507ed9d840c01a3c5b2a39af8bb4000bfac30bf5`, Compatibility/OpenGL 4.6,
Mesa 26.2.4, AMD Custom GPU 0405. Installed Weston 15.0.1-3, monospace 16,
`/bin/sh` (Bash). No host configuration or services changed.

```sh
meson compile -C build
meson test -C build --print-errorlogs
tools/check-godot-binding.sh
tools/check-godot-study.sh
```

- C++ build and 33/33 Meson regressions pass.
- Binding, protocol lifecycle, frames, keyboard and pointer checks pass.
- New `runtime-resize`/`resize-client` tests pass: client isolation, configure
  ordering, pending versus committed geometry/limits, min/max clamping, ack-only
  delay, retained old snapshots, client-chosen sizes, declined resize, duplicate
  suppression, successive configures/newest ack, geometry-only commit, effective
  geometry persistence, 64 pending requests, destruction while pending and stale
  handle rejection after restart. Negative clients verify exact protocol errors
  for stale acknowledgements, negative/conflicting limits and overflowing/outside
  geometry. Expected protocol-error logs are not unexpected test failures.
- The complete resize harness passes AddressSanitizer, UBSan and leak detection.
  This instruments project native code, not Godot itself. Reproduce using the
  prior handoff's sanitizer recipe with `tests/test_runtime_resize.cpp`, output
  `runtime-resize-asan`, fixture `build-godot-probe/resize-client`.
- Godot import/runtime and all scene/controller/color/transform/live-output/
  keyboard/pointer checks pass. Existing keyboard native-pixel comparison now
  samples 11,163 pixels after the larger activation resize. Pointer selection and
  scrollback still pass (7,496 / 1,875 changed-region pixels in this run).
- New real-client `terminal_resize.gd` passes: 1000×700 → 1200×780 → 900×620 →
  1280×800 host sizes produce character grids 77×22 → 95×25 → 68×18 → 103×26.
  From the initial 1280×800 activation, this is two shrinks and two grows.
  Actual buffers change in both dimensions. Shell-written `stty size` files prove
  grid changes; typing and selection work after each resize. 8,573 sampled opaque
  pixels match the native-size GPU view within two bytes/channel. The monitor's
  aspect updates and mode exit/re-entry preserves state without another configure.
- [Five rendered captures](../evidence/terminal-resize/) were inspected. Text is
  upright and retains its size; selection matches the output line, and the room
  monitor shows the resized client. These are agent-injected events and direct
  window-size changes, not physical mouse drag-resize or human usability evidence.

Logs: `.tools/recovery/t44/`; detailed engine logs are in the locations printed
by `import.log` and `study.log`. Images and real shell size files regenerate in
`.tools/terminal-resize-test/`, never over committed historical captures.

## Exact continuation and limits

**P21-T45**: add a targeted runtime `xdg_toplevel.close` request and a documented
Godot close action. The client may accept, defer or refuse it. Do not kill the
process to imitate a close acknowledgement. Add explicit terminal relaunch after
exit with duplicate-launch prevention, fresh live handles and clean input/texture
state. Keep process shutdown distinct from window close. Follow the multiwindow
fixture, real-client and sanitizer acceptance in TASKS.md.

Client cursor images, clipboard/primary selection, drag-and-drop and popup menus
remain unsupported. A right-click menu can still disconnect the terminal because
xdg_popup is rejected. T45's integration review must create concrete follow-up
work for these gaps before full usability acceptance. Broad/human gates remain
open. Very small host windows, large client minimums, HiDPI physical hosts, actual
window-manager drag-resize and latency need separate acceptance; the proven
rendered sizes are listed above. No new dependency or host installation was added.
