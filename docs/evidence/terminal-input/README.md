# Agent-observed terminal keyboard interaction

Captured and inspected 2026-10-03 by Codex for P21-T42, working tree based on
`d7a590c`, committed with the implementation. Godot 4.7.2 Compatibility/OpenGL,
Mesa 26.2.4, AMD Custom GPU 0405, 1280×800 viewport, Weston terminal 15.0.1.
Reproduce with `tools/check-godot-study.sh`; generated captures are under
`.tools/terminal-input-test/` so reruns preserve these historical images.

- `application.png`: GPU-rendered flat view of the live terminal, showing actual
  shell commands/results, key repeat, Ctrl+C interruption, and typed text after
  world/application transitions and a simulated host-focus-loss notification.
- `world-after-close.png`: GPU-rendered study after typing `exit` in the shell
  while Shift is held. The client image is cleared and world mode restored.
- `source.png`: owned pixels from the real terminal at the application capture,
  **not a GPU screenshot**. 7,571 sampled opaque source pixels match the flat
  GPU view within two bytes per channel at native pixel size.

The test injects Godot events through the production input route. Shell-written
files separately establish that typed commands executed. This is agent-run
keyboard evidence, not a physical keyboard, human comfort, latency, pointer or
resize trial. See [handoff 21](../../handoffs/21-godot-terminal-keyboard.md).
