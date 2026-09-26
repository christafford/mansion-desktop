# Decision 02: nested I/O model, shell protocol, and headless verification

**Date:** 2026-09-26. **Status:** accepted. Supersedes the parts of
[01-stack.md](01-stack.md) that describe wlroots as the compositor foundation.

## Context

The code written for Project 1 so far uses libwayland-server directly (no
wlroots), an X11 host window with EGL/GLES2, the deprecated `wl_shell`
protocol, and raw `/dev/input` evdev reading. It builds, but nothing works
end to end, and several choices block Project 1:

- Modern Wayland terminals (`weston-terminal`, `foot`, GTK, Qt) only speak
  `xdg_shell`. They cannot open a window on a compositor that only offers
  `wl_shell`.
- Reading `/dev/input` grabs the whole machine's keyboards and mice while the
  host desktop is still running. Keystrokes typed into other host windows
  are forwarded to nested clients, and the code needs device permissions the
  development container does not reliably have. A nested compositor must take
  input from its host window.
- The launcher gave children a fresh `XDG_RUNTIME_DIR`, so they could never
  find the socket the compositor created in the parent's runtime directory.
- Nothing could be verified without a human watching a window. Autonomous
  sessions with a local model cannot see the screen, so every acceptance
  criterion that requires eyes was either skipped or (worse) reported as
  passed.

## Decisions

1. **Stack.** C++20, Meson, libwayland-server used directly. wlroots is not a
   dependency (it is not installed and the existing code does not use it).
   Reconsider wlroots at Decision gate A (after Project 2) only if buffer
   import, XWayland, or protocol breadth prove too costly by hand.
2. **Host window and input.** The host window is an X11 window (via
   `DISPLAY`, which is available inside the container through XWayland).
   Keyboard, pointer, focus, resize, and close events come from that window's
   X11 events. X11 keycodes minus 8 are evdev keycodes. The `wl_keyboard`
   keymap is built with xkbcommon from `XKB_DEFAULT_*` environment names,
   falling back to `us`. The evdev code is removed rather than extended.
3. **Shell protocol.** `xdg_shell` (`xdg_wm_base`, `xdg_surface`,
   `xdg_toplevel`, popups later) generated from `wayland-protocols` with
   `wayland-scanner` at build time. `wl_shell` is removed.
4. **Buffers.** `wl_shm` buffers first, uploaded to GL textures. Accelerated
   buffers (EGL `WL_bind_wayland_display` or `linux-dmabuf`) are the Project 2
   integration experiment and remain optional until they are demonstrated.
5. **Socket naming.** The compositor never reuses the host's `WAYLAND_DISPLAY`
   for its own socket. Default name is `mansion-<pid>`; `--socket NAME`
   overrides it. The chosen name is printed as `MANSION_SOCKET=<name>` on
   stdout so scripts can pick it up.
6. **Launcher environment.** Children inherit the parent's environment,
   with `WAYLAND_DISPLAY` set to the compositor's socket and `DISPLAY` unset so
   toolkits do not fall back to the host X server. `GDK_BACKEND=wayland` and
   `QT_QPA_PLATFORM=wayland` are set as hints. `XDG_RUNTIME_DIR` is inherited
   unchanged.
7. **Headless verification is mandatory.** Every task that changes behaviour
   ships an automated check runnable with `meson test -C build` and no
   display. The compositor gets a `--headless` mode (no X11 window; offscreen
   EGL when available), `--exit-after-ms`, `--screenshot PATH` (PPM dump), and
   `--input-script FILE` (scripted synthetic input). A small C test client in
   `tests/` exercises protocol behaviour and prints events as lines that
   shell/python tests assert on. Things that genuinely need a human (feel,
   latency, real terminals) are listed as manual acceptance steps and recorded
   as *not observed* until a person runs them.

## Consequences

- `src/shell.cpp` (wl_shell) and the evdev part of `src/input.cpp` are
  replaced, not extended.
- `meson.build` gains protocol generation, a test client, and `test()`
  entries. `tests/` becomes part of the repository.
- Documentation must distinguish "verified by automated test", "verified by a
  person", and "not verified". `docs/STATUS.md` uses exactly those labels.
- Because a local model performs most of the work unattended, tasks are cut
  small in [../TASKS.md](../TASKS.md), each with one acceptance command.
