# Development notes

See [GETTING_STARTED.md](GETTING_STARTED.md) for dependencies and the basic
build. This file covers the environment details that matter when things fail.

## Environment used so far

- Steam Deck (SteamOS, Wayland session) hosting a `distrobox` container named
  `mansion-dev` (Arch Linux, podman). The repository is bind-mounted at
  `/home/deck/code/mansion-desktop`; the container home is
  `/home/deck/distrobox-homes/mansion-dev`.
- Inside the container `DISPLAY=:0` reaches the host through XWayland, so the
  windowed compositor opens a real window on the Deck's screen.
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
- A stuck compositor: `pkill -f build/mansion-desktop`; stale sockets are
  removed on the next clean exit, or by hand from `$XDG_RUNTIME_DIR`.

## Code layout

```text
src/main.cpp          options, startup order, main loop
src/compositor.*      wl_compositor and wl_surface state
src/shell.*           wl_shell (replaced by xdg-shell in P1-T03)
src/display.*         host window, EGL/GLES2, headless stub, screenshots later
src/input.*           wl_seat; evdev to be replaced by host-window input
src/launch.*          child process launcher
tests/                test client, shell tests, helpers
docs/                 STATUS, TASKS, decisions, handoffs
```
