# Terminal pointer evidence — 2026-10-03

Observer: Codex agent. T43 working tree based on `ddd489c`, committed with
[handoff 22](../../handoffs/22-godot-terminal-pointer.md).
Real installed Weston 15.0.1-3, monospace size 16, `/bin/sh` (Bash), private
Wayland socket. Godot 4.7.2 Compatibility/OpenGL 4.6, Mesa 26.2.4,
AMD Custom GPU 0405, 1280×800 viewport.

All four files are GPU viewport captures, inspected by the agent:

- `selection.png`: left-button drag highlights the real terminal's output line.
- `scroll-bottom.png`: `seq 1 100` has finished; rows 77–100 and the shell prompt.
- `scroll-up.png`: three wheel-up events reveal earlier rows 47–71.
- `world-after-drag.png`: Ctrl+Alt+Escape during another drag returns to the
  study; walking resumes and stops on key release. Live terminal stays on the monitor.

Reproduce with `tools/check-godot-study.sh`; generated images go to
`.tools/terminal-pointer-test/`, preserving these historical files. The test
injects Godot events; no rendered pixels or client output are simulated.
It also checks release outside the image, focus-loss notification, disconnect
while held and restoration of scrollback after wheel-down events.

This is bounded pointer/scroll evidence, not a physical mouse, touchpad, latency
or human usability trial. Clipboard/primary selection, client cursor images and
popup menus remain unsupported; see the handoff. Human acceptance: not observed.
