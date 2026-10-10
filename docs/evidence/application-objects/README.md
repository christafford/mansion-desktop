# Live application objects — agent evidence

2026-10-05, Codex agent; source `88f9938` plus P21-T49 and existing uncommitted
work. Godot 4.7.2 Compatibility/OpenGL 4.6, Mesa 26.2.4, AMD Custom GPU 0405.
GPU viewport 800×800 on the current 800×1280 portrait KWin desktop. Default
study arrival camera: player spawn `(2.4, 0.05, 3.1)`, yaw `0.38`, pitch `-0.08`
(normal floor settling applies). No synthetic application images.

- [Two live applications](two-live-applications.png): installed Vim and a real
  Weston terminal executing a changing clock loop. Panels have separate client
  pixels and world entities. Vim was launched through the existing application
  launcher. The gold accent marks the selected panel.
- [After movement](moved-applications.png): injected drag/wheel moves Terminal;
  Vim retains its placement. Double-click returns to Terminal, Ctrl+C stops its
  loop, and a typed command writes the asserted `TARGET` file before room return.

Reproduce with the graphical `world/tests/application_objects.gd` test. Text is
a world preview; ordinary typing uses the existing full-size application view.
These are agent-inspected renders and injected input, not a human or physical
input/latency trial. Session placement is not saved across Elsewhere restarts.
See [handoff 26](../../handoffs/26-application-objects.md) for test commands,
regression limits and the known pre-existing navigation failure.
