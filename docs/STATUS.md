# Project status

Current authority: 2026-10-03 recovery, starting from `4c0c078` plus the existing
uncommitted overnight work. [Decision 06](decisions/06-godot-poly-haven.md) remains
the direction. [Previous status](handoffs/15-status-before-frontend-recovery.md)
is historical; its extension/visual completion claims were contradicted by tests.

## Next task

**P21-T42: type into the real terminal in Godot application mode.**
Recovery T36–T41 passes its bounded acceptance: furnished study, standard
binding, real Wayland protocols, owned pixels and changing terminal output on
the monitor. See [tasks](TASKS.md) and
[live output handoff](handoffs/20-godot-live-terminal.md).
Next expose seat keyboard focus/delivery, implement explicit application mode,
and verify actual shell commands. Do not treat the output preview as a usable
desktop. Pointer/resize/client-close acceptance follows in T43.
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
  These checks alone do not establish rendered usability.
- Live output: two real Weston terminal sessions, changing pixels/revisions,
  aspect ratio, unchanged-frame skip, disconnect clearing, child/socket cleanup
  and unchanged host display environment pass. Eight inverse image transforms
  and eleven GPU color patches under strong colored lighting pass.
- Standalone core/runtime AddressSanitizer + UBSan + enabled leak detection
  passed during T40; T41 changes window enumeration and presentation.
- C++ build and 33/33 Meson tests pass; 62/62 Node plugin tests passed during T39.
  They do not validate live Godot compositor integration.
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

## Verified by a person

Historical owner confirmation on 2026-10-02: legacy launch, input and vertical
orientation fixes worked. Owner response on 2026-10-03 to the recovered view: “finally! that looks great.”
This records approval of its appearance, not a terminal/input/comfort trial.

## Not verified / unfinished

- Real application input, focus transitions/application mode, resize, client
  close requests and an in-app relaunch workflow. Live output and owned process
  shutdown are verified; ordinary terminal usability is not.
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
