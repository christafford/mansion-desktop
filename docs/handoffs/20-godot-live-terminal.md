# Live terminal output in the study — 2026-10-03

P21-T41 builds on `3a8843f` plus the preserved overnight tree. The study now
starts the recovered compositor and displays changing output from a real Weston
terminal on its monitor. This passes the live-output gate. Application input,
focus mode, resize, client close and general desktop usability remain unfinished.

## Run and ownership

```sh
tools/run-godot.sh --audio-driver Dummy
# Optional ordinary terminal program showing a changing clock/counter:
tools/run-godot.sh --audio-driver Dummy -- --terminal-demo
```

The default launches `/usr/bin/weston-terminal --font=monospace --font-size=16
--shell=/bin/sh`. The demo substitutes executable `world/tools/terminal-demo.py`
as its shell; Python writes ANSI text to the real terminal's PTY. It has no
Mansion API and supplies no image. The C++ core receives actual Wayland shm
buffers from Weston. There is no screenshot replay or fake terminal widget.

`tools/build-godot-runtime.sh` builds the existing standard godot-cpp target and
atomically copies it to `world/addons/mansion_runtime/libmansion_runtime.so`.
The run and project validation helpers invoke it. Generated libraries stay
untracked. The crashing `mansion_godot` addon remains ignored and preserved.
This uses the already-installed Weston 15.0.1-3; missing executables fail visibly,
without system installation or fallback to a synthetic client.

`terminal_screen.gd` owns one session and one child PID. It supplies the private
absolute `WAYLAND_DISPLAY` only to that child, clears inherited display/socket
overrides, and uses repository-local isolated terminal configuration. The host
Godot environment is unchanged. The scene pumps the compositor once per frame.
Ordinary shutdown stops the server, gives the child 1.5 seconds to disconnect,
then terminates only that owned child if necessary. Scene teardown also cleans
up. This launcher cleanup is separate from future `xdg_toplevel.close` policy.

## Presentation

The runtime's `toplevel_handles()` filters out surfaces without an xdg toplevel
role. The study selects its first live toplevel; this is not a persistent window
registry or multi-window shell. `snapshot(handle, after_revision)` returns an
empty dictionary for unchanged content, avoiding another byte-array copy.
The existing one-argument snapshot API remains supported.

The screen applies the inverse Wayland buffer transform, keeps native texture
resolution, and fits logical aspect ratio inside the monitor aperture. The
860×658 Weston window therefore has side margins in the wider monitor. New
dimensions replace ImageTexture; new pixels at the same dimensions update it.
Detach/removal hides the image and displays inactive/closed status; old snapshots
are never retained as a supposedly live application.

Godot's unshaded material still receives the world's Filmic tone mapping. Initial
GPU measurements altered client gray 128 to 162. The screen shader now uses a
256-entry float inverse lookup, generated once from the pinned Compatibility
shader curve, before the room's tone mapping. It preserves client sRGB without
changing the room appearance. This is deliberately tied to Compatibility,
Filmic, exposure=1, white=1; rerun GPU color checks when changing renderer or
environment. Transparent margins composite over black. No HDR/general color
management, DMA-BUF import or zero-copy claim is made.

## Verification and evidence

Observer: Codex agent, 2026-10-03, T41 working tree based on `3a8843f` (this handoff
is committed with the implementation). Godot `4.7.2.stable.official.ed1daf0bf`,
godot-cpp `507ed9d840c01a3c5b2a39af8bb4000bfac30bf5`, Compatibility/OpenGL 4.6,
Mesa 26.2.4, AMD Custom GPU 0405, 1280×800 captures. Engine identity/provenance
limits remain in [TOOLCHAIN.md](../TOOLCHAIN.md); no new upstream provenance claim.

```sh
meson compile -C build
meson test -C build --print-errorlogs
tools/check-godot-binding.sh
tools/check-godot-study.sh
```

- C++ build and 33/33 Meson tests pass.
- Standard binding: 300 probe calls, 16 native + 16 Godot protocol trials,
  32 native + 32 Godot frame barriers and six negative frame cases per harness
  pass. New assertions check toplevel enumeration and revision filtering.
- Project import/runtime validation passes with the documented paced-import
  workaround. It remains an engine-workaround check, not visual acceptance.
- Controller smoke passes: 56 textured surfaces plus movement, collision,
  capture/release, host focus cleanup and Home assertions. Events are injected.
- All eight inverse image transforms pass independent asymmetric permutations.
- Eleven synthetic GPU patches preserve exact captured gray/RGB bytes in this
  run, under strong red directional lighting; assertion tolerance is two bytes.
  These patches are explicitly test fixtures, not terminal evidence.
- Real terminal integration passes two sessions: changing owned pixels/revisions,
  fitted aspect ratio, unchanged-revision skip, abrupt child exit clearing, then
  orderly shutdown of another live terminal. Child processes and private socket
  directories are gone; the host display environment is unchanged.

`tools/check-godot-study.sh` checks return codes, error logs and success markers;
it requires a graphical display and retains per-run logs under `/tmp`.
Session logs are `.tools/recovery/t41/`. Weston warns about unavailable
`dnd-copy`/`dnd-none` cursor images; output, cleanup and assertions still pass.
No new sanitizer run is claimed; the core ownership path was sanitizer-tested
in T40, while T41 adds enumeration and presentation.

Inspected [GPU captures](../evidence/live-terminal/): `room.png`,
`monitor-before.png`, `monitor-after.png` and `closed.png`. They show the furnished
room, upright readable-at-close-up text, a changed clock/counter, and cleared
content after exit. `client-frame.png` is the actual owned source image, **not** a
GPU screenshot. The world preview is small at walking distance; readable flat
application mode remains T42. These observations do not establish physical-input
behavior, latency, final art quality or human usability. No human task is ticked.

## Next bounded work

P21-T42: reuse the owned seat/runtime and implement keyboard focus/delivery,
explicit application mode, a readable flat live view and Ctrl+Alt+Escape return.
Prove real shell-command input before adding pointer/resize/client-close in T43.
The detailed instructions and dependencies are in [TASKS.md](../TASKS.md).
Broader reopened gates remain open. Do not revive the ignored raw-ABI addon,
replace the compositor, or sweep unrelated overnight edits into the next commit.
