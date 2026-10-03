# Project status

Current authority: 2026-10-03 recovery, starting from `4c0c078` plus the existing
uncommitted overnight work. [Decision 06](decisions/06-godot-poly-haven.md) remains
the direction. [Previous status](handoffs/15-status-before-frontend-recovery.md)
is historical; its extension/visual completion claims were contradicted by tests.

## Next task

**P21-T38: connect the recovered binding to a verified compositor lifecycle.**
P21-T36 frontend recovery and P21-T37 minimal standard binding pass their bounded
acceptance. See [tasks](TASKS.md), [frontend handoff](handoffs/14-godot-frontend-recovery.md)
and [binding handoff](handoffs/16-godot-binding-recovery.md). Next prove a private
socket, actual fixture client, nonblocking server pump and clean shutdown before
adding pixel delivery. The old addon remains isolated.
Broader T00–T19 gates remain unchecked until their complete evidence exists;
these recovery subtasks do not pass the full product or provenance gates.

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
- C++ build and 31/31 Meson tests pass; 62/62 Node plugin tests pass.
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
orientation fixes worked. No owner acceptance of the recovered Godot scene yet.

## Not verified / unfinished

- Live terminal integration, real application input, focus transitions, resize,
  close/relaunch and owned snapshots in Godot.
- Original addon initialization: reproduced segmentation fault; isolated with
  `world/addons/mansion_godot/.gdignore`, source and binary preserved.
- Full renderer-independent compositor extraction: existing library/test are
  partial; fixture socket/seat/xdg lifecycle acceptance remains open.
- Finished art rubric, furniture selection/cohesion, performance, physical host
  input and human comfort. No blank capture counts as visual acceptance.
- Fresh-clone dependency provenance and asset fetching/offline negative checks.

Unrelated overnight edits remain in the working tree. Recovery backup:
`.tools/recovery/20261003T185319Z/`. Never sweep them into a recovery commit.
