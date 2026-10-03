# Godot frontend recovery — 2026-10-03

Owner authorized recovery following the audit of HEAD `4c0c078` plus extensive
uncommitted overnight work. That input was preserved: a binary tracked diff,
small edited/untracked sources, and the previous generated Godot cache are in
`.tools/recovery/20261003T185319Z/`. No C++ work was reset or replaced.

## P21-T36 result

The real Godot project now imports complete glTF sources and renders all six
furniture models. Source cache and the crashing experimental addon have
`.gdignore` boundaries. The old cached extension list was moved into the recovery
snapshot; a clean import produced the current generated cache.

The scene now has correct material objects, light and shadows, imported models,
a monitor visibly labelled as an inactive preview, and one CharacterBody3D
controller. Mesh collisions derive from the displayed furniture. WASD walks,
hold right mouse looks, release/Escape releases, focus loss clears held keys,
Home restores spawn, M/Shift slow walking. No fake application mode remains.

## Verification collected by the agent

- `python3 world/tools/prepare_study_assets.py`: six complete source packages;
  model, buffer and texture SHA-256 values verified against the existing manifest.
  No downloads. This does not verify upstream provenance or fresh-clone fetching.
- `tools/validate-godot-project.sh`: import and five-frame headless runtime pass.
  Validation uses correct project paths, bounded processes and unique logs;
  script/resource errors fail even if Godot returns zero.
- Intentionally invalid script project under `.tools/recovery/invalid-project`:
  validator exits 1 with the actual parse error, as required.
- `tools/run-godot.sh --audio-driver Dummy --script res://tests/study_smoke.gd`:
  56 textured mesh surfaces; movement/release, wall collision, focus cleanup,
  mouse capture/look/release and Home assertions pass. Input is injected into
  the controller. A headless attempt correctly failed capture assertions because
  the dummy display cannot capture a pointer; graphical execution is required.
- GPU-rendered arrival, desk and opposite-corner images are under
  [evidence](../evidence/godot-recovery/). The agent inspected the actual images.
  The first views revealed backward desk/chair orientation, corrected before
  the final captures. These are real engine renders, not generated concept art.
- Renderer: Compatibility/OpenGL 4.6, Mesa 26.2.4-arch1.1,
  AMD Custom GPU 0405. This is device identity, not a performance benchmark.
- Earlier same-session audit: `meson compile -C build` succeeded and all 31
  Meson tests passed. This task changes no C++ implementation.

## Limits and continuation

The industrial desk and worn shelves are usable recovery assets, not acceptance
of the warm luxurious study rubric. Floor/rug detail, shelf dressing, material
cohesion, lighting refinement and performance remain pending. Host-delivered
physical input and human comfort are not established by injected controller tests.

No live terminal, real application input, resize or bridge lifetime acceptance.
The old extension crashed in class registration and remains preserved but ignored.
Next: P21-T37 minimal standard binding; then finish T00/T10 prerequisites and
prove T11 lifecycle before implementing snapshot/texture delivery.
