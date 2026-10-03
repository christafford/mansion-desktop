# Project status

Current authority: 2026-10-03 recovery, starting from `4c0c078` plus the existing
uncommitted overnight work. [Decision 06](decisions/06-godot-poly-haven.md) remains
the direction. [Previous status](handoffs/15-status-before-frontend-recovery.md)
is historical; its extension/visual completion claims were contradicted by tests.

## Next task

**P21-T41: present live terminal output on the Godot study monitor.**
Recovery T36–T40 now passes its bounded acceptance: visible study, standard
binding, real Wayland protocols and owned client pixels through Godot. See
[tasks](TASKS.md) and [frame handoff](handoffs/19-godot-owned-frames.md).
Next connect the tested session and ImageTexture to the study, launch a real
native terminal and inspect actual changing output. The study still displays
an inactive monitor; its scene does not yet start the tested server.
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
`build/mansion-desktop --room-camera` still uses the separate legacy renderer.

WASD walks; hold right mouse to look; release/Escape frees the pointer; Home
returns to arrival; M toggles slow walking and Shift slows while held.

## Verified by automated checks in this recovery

- Six glTF packages staged with their manifest-checked buffer/texture dependencies.
- Godot import and runtime pass; validator rejects an invalid-script fixture.
- Graphical scene smoke: 56 textured surfaces, walking/key release, wall
  collision, focus cleanup, mouse capture/look/release and Home assertions pass.
  These are injected controller events, not a physical-input or human trial.
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
  These checks do not render application content on the study monitor.
- Standalone core/runtime AddressSanitizer + UBSan + enabled leak detection pass.
- C++ build and 33/33 Meson tests pass; 62/62 Node plugin tests passed during T39.
  They do not validate live Godot compositor integration.
- The local editor's first-import shutdown race has a tested paced-startup
  workaround, not an engine fix; see [TOOLCHAIN.md](TOOLCHAIN.md).

## Agent-observed visuals

[Three rendered views](evidence/godot-recovery/) show real desk, chair, lamp,
plant, books and shelving, room textures, daylight/shadows and an inactive
monitor preview. Captures were inspected, including correction of the initially
backward desk/chair. Compatibility/OpenGL on AMD Custom GPU 0405.
This passes visible frontend recovery, not final art acceptance.

## Verified by a person

Historical owner confirmation on 2026-10-02: legacy launch, input and vertical
orientation fixes worked. Owner response on 2026-10-03 to the recovered view: “finally! that looks great.”
This records approval of its appearance, not a terminal/input/comfort trial.

## Not verified / unfinished

- Live terminal integration, real application input, focus transitions, resize,
  close/relaunch and presentation of the owned snapshots on the monitor.
- Original addon initialization: reproduced segmentation fault; isolated with
  `world/addons/mansion_godot/.gdignore`, source and binary preserved.
- Full renderer-independent compositor extraction: socket/wl_compositor/shm
  lifecycle, initial xdg-shell/seat checks and owned shm snapshots pass. Broader
  surface-tree/protocol conformance and Godot input delivery remain open.
- Finished art rubric, furniture selection/cohesion, performance, physical host
  input and human comfort. No blank capture counts as visual acceptance.
- Fresh-clone dependency provenance and asset fetching/offline negative checks.

Unrelated overnight edits remain in the working tree. Recovery backup:
`.tools/recovery/20261003T185319Z/`. Never sweep them into a recovery commit.
