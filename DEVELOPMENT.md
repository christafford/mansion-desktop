# Development notes

See [GETTING_STARTED.md](GETTING_STARTED.md) for dependencies and the basic
build. This file covers the environment details that matter when things fail.

## Current frontend work

Decision 06 and Project 21 now authorize Godot's nested host window/scene with a
C++ GDExtension core. See GODOT-INTEGRATION.md and the pinned TOOLCHAIN.md record.
The environment notes below describe the supplied prototype. Preserve its
headless tests during extraction, but do not wait for the old host-renderer
recovery tasks to author the Godot room. Host input must come from the engine's
focused window; global evdev scanning must not enter the new frontend.
Record actual Distrobox host socket/driver access and terminal private-socket
launch separately. No native session or host service changes are authorized.

## Environment used so far

- Steam Deck (SteamOS, Wayland session) hosting an Arch Linux `distrobox`
  container through podman. The repository is bind-mounted at
  `/home/deck/code/elsewhere`; use `$HOME` for the container home.
  The project rename does not rename the existing host container or its home.
- Historical configuration: `DISPLAY=:0` reached the host through XWayland and
  was unstable. Since 2026-09-28 the windowed mode connects to the host
  `WAYLAND_DISPLAY` instead; that path is unverified (reported flicker, no
  input). P4-T12 investigates it while preserving headless. Record actual
  display/socket availability rather than assuming it.
- `XDG_RUNTIME_DIR=/run/user/1000` is shared with the host. Never create a
  socket named like the host's `WAYLAND_DISPLAY` (`wayland-0`); the compositor
  defaults to `elsewhere-<pid>` for this reason, and tests use a private
  temporary runtime directory.
- `/dev/dri/renderD128` is accessible, so EGL can use the GPU; llvmpipe is the
  fallback.
- `sudo` requires a person. Autonomous sessions do not install packages.

## Build variants

```sh
meson setup build --buildtype=debug
meson setup build-asan -Db_sanitize=address,undefined --buildtype=debug
meson compile -C build-asan
meson test -C build-asan --print-errorlogs
```

`build*/` directories are git-ignored.

## Docker

`Dockerfile.dev` builds an Arch image with the dependencies:

```sh
docker build -t elsewhere-dev -f Dockerfile.dev .
docker run -ti --rm -v "$PWD:/src" -w /src elsewhere-dev sh -c \
    'meson setup build && meson compile -C build && meson test -C build'
```

Windowed mode inside Docker needs the host Wayland socket forwarded; the
headless tests do not.

## Debugging

- `WAYLAND_DEBUG=1` on either side prints protocol traffic.
- `meson test -C build <name> --verbose` shows the test script's stdout; the
  shell helpers dump compositor stderr on failure.
- Tests leave nothing behind: each creates and removes its own runtime dir.
- A stuck trial: terminate only the PID you started; do not kill other Elsewhere
  sessions by a broad pattern. Remove only that trial's private stale socket/lock
  after confirming its owner exited. Never remove the host's Wayland socket.

## Code layout

```text
src/main.cpp          options, startup order, main loop
src/compositor.*      wl_compositor and wl_surface state
src/xdg-shell.*       xdg_wm_base, xdg_surface, xdg_toplevel
src/display.*         host window/EGL, GLES2, camera/panel/room, screenshots
src/input.*           wl_seat, host-window events, evdev camera input, scripted input and modes
src/launch.*          child process launcher
tests/                test client, shell tests, helpers
docs/                 STATUS, TASKS, decisions, handoffs
```
