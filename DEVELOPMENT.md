# Development notes

See [GETTING_STARTED.md](GETTING_STARTED.md) for dependencies and the basic
build. This file covers the environment details that matter when things fail.

## Environment used so far

- Steam Deck (SteamOS, Wayland session) hosting a `distrobox` container named
  `mansion-dev` (Arch Linux, podman). The repository is bind-mounted at
  `/home/deck/code/mansion-desktop`; the container home is
  `/home/deck/distrobox-homes/mansion-dev`.
- Historical configuration: `DISPLAY=:0` reaches the host through XWayland.
  Archive STATUS reports unstable host-window behavior; this is not a verified
  reliable backend. P4-T12 investigates a native Wayland host while preserving
  X11/headless. Record actual display/socket availability rather than assuming it.
- `XDG_RUNTIME_DIR=/run/user/1000` is shared with the host. Never create a
  socket named like the host's `WAYLAND_DISPLAY` (`wayland-0`); the compositor
  defaults to `mansion-<pid>` for this reason, and tests use a private
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
docker build -t mansion-desktop-dev -f Dockerfile.dev .
docker run -ti --rm -v "$PWD:/src" -w /src mansion-desktop-dev sh -c \
    'meson setup build && meson compile -C build && meson test -C build'
```

Windowed mode inside Docker needs the host X socket or XWayland forwarded;
the headless tests do not.

## Debugging

- `WAYLAND_DEBUG=1` on either side prints protocol traffic.
- `meson test -C build <name> --verbose` shows the test script's stdout; the
  shell helpers dump compositor stderr on failure.
- Tests leave nothing behind: each creates and removes its own runtime dir.
- A stuck trial: terminate only the PID you started; do not kill other Mansion
  sessions by a broad pattern. Remove only that trial's private stale socket/lock
  after confirming its owner exited. Never remove the host's Wayland socket.

## Code layout

```text
src/main.cpp          options, startup order, main loop
src/compositor.*      wl_compositor and wl_surface state
src/xdg-shell.*       xdg_wm_base, xdg_surface, xdg_toplevel
src/display.*         host window/EGL, GLES2, camera/panel/room, screenshots
src/input.*           wl_seat, host X11 events, scripted input and modes
src/launch.*          child process launcher
tests/                test client, shell tests, helpers
docs/                 STATUS, TASKS, decisions, handoffs
```
