# Application launcher evidence

Agent-observed, 2026-10-04, T48 based on `7316f78` plus its committed changes.
Godot 4.7.2, Compatibility/OpenGL 4.6, Mesa 26.2.4, AMD Custom GPU 0405,
1280×800. Captured by `world/tests/app_launcher.gd` through the rendered viewport.

- `two-recent-applications.png`: actual Vim launch followed by the earlier
  Terminal, clockwise from the top. Remaining history slots are empty.
- `application-search.png`: actual installed application names and resolved icons.
- `live-editor.png`: real Vim running inside Weston, with injected typed text.
- `relaunched-terminal.png`: newly launched real shell after the original exited;
  the test separately reads its output file to verify typing.
- `fixture-eight-slots.png`: **injected history for layout only**, visibly labeled
  in the capture. It uses real catalog icons but does not claim these eight
  applications were launched or are compatible.

These are injected-input checks and agent visual inspection, not a human usability
trial. See [handoff 25](../../handoffs/25-application-launcher.md) for exact checks,
failures corrected and remaining compatibility limits. Reproduce with
`tools/check-godot-study.sh`; generated captures are in
`.tools/launcher-test-captures/`.
