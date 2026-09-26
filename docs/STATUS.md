# Project status

Updated 2026-09-26. This file is the single source of truth for what works.
Use exactly these labels: **verified by automated test**, **verified by a
person**, **not verified**. Never move an item to a "verified" list without
running the check that proves it.

## Next task

**P1-T03 xdg-shell replaces wl_shell** in [TASKS.md](TASKS.md). Prerequisites
(P1-T01, P1-T02) are ticked and pass with `meson test -C build`.

Continuation point: start by adding `wayland-scanner` code generation to
`meson.build` (the commented block shows the shape), then create
`src/xdg-shell.cpp` implementing `xdg_wm_base`, `xdg_surface`, `xdg_toplevel`,
then extend `tests/mansion-test-client.c` with `--toplevel`, then add
`tests/client_toplevel.sh` and its `meson.build` entry. Delete `src/shell.cpp`
last, once the new test passes.

## Verified by automated test

Run `meson test -C build --print-errorlogs`.

- `smoke-headless` (P1-T01): `mansion-desktop --headless --socket NAME
  --exit-after-ms N` starts without a display, prints `MANSION_SOCKET=NAME`,
  creates the socket and lock file in `$XDG_RUNTIME_DIR`, exits 0 on its own,
  removes both files, and opens no input devices.
- `client-globals` (P1-T02): `mansion-test-client` connects and sees
  `wl_compositor`, `wl_shm`, `wl_seat`; the compositor survives the client
  disconnecting and exits 0 on SIGTERM.
- Auto-continue plugin: `node --test .opencode/tests/*.test.js` (34 tests),
  plus one live run against OpenCode 2.0.16 with the local model on
  2026-09-26 (see [OPENCODE-AUTOCONTINUE.md](OPENCODE-AUTOCONTINUE.md)).

## Verified by a person

Nothing yet. No real Wayland client has ever opened a window on this
compositor. `weston-terminal` is not installed in the development container
(`sudo pacman -S weston` provides it).

## Not verified / known broken

- **No client can map a window.** Only the deprecated `wl_shell` is offered;
  every current toolkit and terminal requires `xdg_shell` (P1-T03).
- **Buffers are not drawn.** Committed `wl_shm` buffers are tracked as a
  pointer only; the renderer draws untextured placeholder quads with pixel
  coordinates fed to a clip-space shader, so nothing sensible appears (P1-T04,
  P1-T05). No frame callbacks are sent, so clients that wait for `frame` stall.
- **Input comes from `/dev/input`** in windowed mode and is forwarded to the
  single most recent keyboard/pointer resource without focus, enter/leave,
  modifiers, or serials. It reads the host machine's real keyboards while the
  host desktop is running. Headless mode skips it. Replacement: P1-T06, P1-T07.
- **Seat handles one client badly.** A second `get_keyboard` posts a protocol
  error; `wl_seat` binds overwrite each other (P1-T06).
- **Launcher** works for simple commands (whitespace split, environment set,
  `DISPLAY` unset) but children are not reaped, `--launch` errors are only
  logged, and exit status is not reported (P1-T08).
- **Host window** has no event handling: no resize, no close button, no focus
  tracking (P1-T07).
- The X11 `Display*` and `Window` created in `create_display` are never stored
  or destroyed; they leak until process exit.

## Environment facts (development container `mansion-dev`, Arch Linux)

- meson 1.x, ninja 1.13, g++ (C++20), wayland 1.26, wayland-protocols 1.49,
  libxkbcommon 1.13.2, mesa 26.2, libx11 1.8.13, node 26. `wayland-scanner`
  present. `/dev/dri/renderD128` is world-accessible; `DISPLAY=:0` reaches the
  host through XWayland.
- Missing: wlroots, weston/weston-terminal, foot, Xvfb, eglinfo, wayland-info.
  Installing packages needs `sudo`, which autonomous sessions must not use.
  Ask a person: `sudo pacman -S weston`.
- Host: Steam Deck (SteamOS) running a Wayland session; OpenCode 2.0.16 runs in
  the container against a llama.cpp server on `127.0.0.1:8080` (Qwen3.6 35B A3B).

## Commands

```sh
meson setup build --buildtype=debug   # first time
meson compile -C build
meson test -C build --print-errorlogs
./build/mansion-desktop --help
./build/mansion-desktop --headless --exit-after-ms 1000
./build/mansion-desktop --launch weston-terminal        # needs a host DISPLAY and weston
```

## Files

- `src/main.cpp` — options, startup/shutdown order, event loop
- `src/compositor.cpp`, `compositor-private.h` — `wl_compositor`, `wl_surface` state
- `src/shell.cpp` — `wl_shell` (to be replaced by xdg-shell in P1-T03)
- `src/display.cpp` — X11 host window, EGL/GLES2 renderer, headless stub
- `src/input.cpp` — `wl_seat`, evdev reading (to be replaced in P1-T06/T07)
- `src/launch.cpp` — child process launcher
- `tests/` — headless test client and shell tests ([tests/README.md](../tests/README.md))
- `docs/TASKS.md` — task list; `docs/handoffs/` — per-project handoffs;
  `docs/decisions/` — recorded decisions
