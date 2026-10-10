# Terminal resize evidence — 2026-10-04

Observer: Codex agent. P21-T44 working tree based on `2e304a9`, committed with
[handoff 23](../../handoffs/23-godot-terminal-resize.md). Real Weston 15.0.1-3,
monospace 16, `/bin/sh` (Bash), private Wayland socket. Godot 4.7.2
Compatibility/OpenGL 4.6, Mesa 26.2.4, AMD Custom GPU 0405.

These are inspected GPU viewport captures, not source-buffer images:

| Capture | Host viewport | Terminal columns × rows (`stty size`) |
| --- | --- | --- |
| `resize-0.png` | 1000×700 | 77×22 |
| `resize-1.png` | 1200×780 | 95×25 |
| `resize-2.png` | 900×620 | 68×18 |
| `resize-3.png` | 1280×800 | 103×26 |
| `world-after-resize.png` | 1280×800 | Updated live monitor after return to room |

Each application capture shows new shell output and text selected using the
resized view's pointer mapping. The shell writes its actual rows/columns to
files; no content is synthesized by Elsewhere. After initial activation at
1280×800, the sequence shrinks twice and grows twice. New buffers are committed,
not merely stretched. Across these four captures, 8,573 sampled opaque client
pixels match within two bytes/channel at native size.

Reproduce with `tools/check-godot-study.sh`; generated images and shell-written
size files go to `.tools/terminal-resize-test/`, preserving historical evidence.
This is agent-injected window/input verification, not physical drag-resize,
latency, human comfort or general application compatibility. Clipboard, client
cursor images and popup menus remain unsupported. Human trial: not observed.
