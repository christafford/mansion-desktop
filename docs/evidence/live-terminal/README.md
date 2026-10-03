# Agent-observed live terminal output

Captured and inspected 2026-10-03 by Codex for P21-T41, working tree based on
`3a8843f`; committed alongside the implementation. Godot 4.7.2 Compatibility,
Mesa 26.2.4, AMD Custom GPU 0405, 1280×800 host viewport, Weston terminal 15.0.1.
Reproduce with `tools/check-godot-study.sh` from the repository root.

- `room.png`: GPU-rendered furnished room with the live terminal on its monitor.
- `monitor-before.png`, `monitor-after.png`: GPU close-ups; the terminal program's
  UTC clock and counter change. No keyboard input is simulated by that program.
- `closed.png`: GPU view after the owned terminal process exits; no stale frame.
- `client-frame.png`: the real terminal's owned 860×658 source pixels, **not a
  GPU capture**. Included to distinguish source quality from world projection.

World camera `(2.4, 1.65, 3.1)` looks at `(0, 1, -2)`. Close-up camera
`(0, 1.26, -1.38)` looks at `(0, 1.2, -2.2)`. Text is upright; the source aspect
ratio is preserved with side margins. See [handoff 20](../../handoffs/20-godot-live-terminal.md)
for test scope and limitations. Output evidence does not pass human or input gates.
