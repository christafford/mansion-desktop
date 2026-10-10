# Movable live application panels — 2026-10-05

P21-T49 is the owner's bounded request, based on `88f9938` plus pre-existing
uncommitted work. It does not authorize a broad roadmap run or satisfy T45.

## Interaction and implementation

Run `tools/run-godot.sh --audio-driver Dummy`. Mapped windows receive independent
live panels in the room. Single-click selects; double-click activates the chosen
window in the existing readable application view. Ctrl+Alt+Escape returns to the
same placement. Left-drag moves across the view; wheel while held changes depth.
Escape cancels. Panels remain upright at their initial facing direction.

The new world manager owns temporary window-to-entity bindings, independent random
entity IDs and scene transforms. It uses existing compositor snapshots; C++ and
the user's dirty navigation files are untouched. Inactive *live* clients keep
updating independently. The selected object shares the monitor's texture;
other textures update by revision. Selected frames still incur a second snapshot
copy. All previews reuse the tested orientation and unlit color shader.

Picking respects nearest objects, furniture/walls and opaque panel backs. Whole
panel bounds stay inside the room; proposed drag positions overlapping furniture,
the player or other panels are rejected. New panels prefer clear visible space;
an overfull room may fall back to overlapping panels near the user. These are
movable UI panels, not physical walking obstacles. No rotation tool is added.
GUI release, focus loss, launcher entry and destruction release drag ownership.
Unmapped content is hidden, and destroyed windows remove their panels/textures.
Session-only placement is not durable artifact storage or restart restoration.

## Fresh evidence

Codex agent, Godot `4.7.2.stable.official.ed1daf0bf`, Compatibility/OpenGL 4.6,
Mesa 26.2.4, AMD Custom GPU 0405. Real Weston 15.0.1 and installed Vim through
T48's launcher. Injected input is not physical input or human acceptance.

- `tools/validate-godot-project.sh`: import/runtime pass; native extension builds.
- `world/tests/application_objects.gd`: two real clients, changing background
  pixels/revisions, independent entities/textures/transforms, click versus double
  click, targeted shell-written marker, drag/wheel, retained placement, Escape,
  launcher and focus cancellation, GUI release, nearest/back/room occlusion,
  room bounds, rejected overlap, actual shell exit while dragging, surviving Vim
  input and owned shutdown. `APPLICATION_OBJECTS_OK clients=2 failures=0`.
- Color, two-session output, ordinary keyboard, selection/scroll and launcher
  tests pass separately, as do 8 transform, 27 key and 302 pointer-map assertions
  and six Python launcher tests. Logs: `.tools/t49-regressions/`.
- Extended terminal resize test verifies world-object texture dimensions and
  aspect alongside existing client reflow/native-pixel assertions. Four sizes
  pass in the isolated virtual desktop: 77×22, 95×25, 68×18, 103×26 character
  grids and 8,573 native pixel samples. This is GPU-rendered Godot, not Godot
  headless mode. Log: `.tools/t49-virtual-resize.log`.
- [Host captures](../evidence/application-objects/) inspected at 800×800: both
  live panels are visible, upright and framed distinctly; moving Terminal leaves
  Vim in place. Initial inspection exposed oversized borders and a clipped
  second panel. Frames now fit their content, and initial placement prefers
  fully visible space. Detailed text uses the full-size application view.

Run the object trial directly without changing normal launcher history:

```sh
ELSEWHERE_LAUNCHER_STATE_DIR="$PWD/.tools/t49-object-state" \
tools/Godot_v4.7.2-stable_linux.x86_64 --path world --audio-driver Dummy \
  --max-fps 60 --script res://tests/application_objects.gd
```

The full `tools/check-godot-study.sh` run is **not green**. It stops at the
pre-existing uncommitted navigation assertion `Pitch return fights mouse after
re-grabbing`. A scratch copy of the current controller test, instantiating the
HEAD study (without this feature), reproduces the same failure. Scratch files
were deleted; `.tools/t49-baseline-controller.log` and `.tools/t49-study-suite.log`
retain evidence. The user's navigation changes/tests were not altered or staged.

The normal host resize check also fails because `_NET_DESKTOP_GEOMETRY` currently
reports 800×1280 and KWin clamps requested width to 800. This is why initial host
captures are square. No display settings or host services were changed. The
isolated resize command was:

```sh
ELSEWHERE_LAUNCHER_STATE_DIR="$PWD/.tools/t49-virtual-state" \
timeout -k 3s 60s weston --backend=headless --renderer=gl \
  --width=1600 --height=1000 --socket=elsewhere-t49-resize --no-config \
  --idle-time=0 --log="$PWD/.tools/t49-weston.log" -- \
  /usr/bin/env WAYLAND_DISPLAY=elsewhere-t49-resize \
  tools/Godot_v4.7.2-stable_linux.x86_64 --display-driver wayland \
  --path world --audio-driver Dummy --max-fps 60 \
  --script res://tests/terminal_resize.gd
```

The private Weston instance exits with its test child. Missing optional host
Wayland protocols and a GTK seat warning are logged; resize assertions still
pass. Existing Weston cursor/escape-sequence warnings remain, without implying
complete Vim emulation or graphical-app compatibility. No new dependencies were
installed. No fresh Meson/sanitizer result is claimed; no C++ code changed.

## Next bounded task

P21-T45 remains next: targeted xdg close, with deferred/declined close and
multiwindow lifecycle acceptance. The navigation assertion needs reconciliation
with the owner's existing edits before the full suite can be green. Broader
human, persistence, popup/cursor/clipboard and product gates stay open.
