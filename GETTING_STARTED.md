# Getting started

## Dependencies

Arch Linux (the development container used so far):

```sh
sudo pacman -S --needed meson ninja gcc pkgconf wayland wayland-protocols \
    libxkbcommon mesa libx11 nodejs
sudo pacman -S --needed weston      # optional: weston-terminal for the human smoke test
```

Debian/Ubuntu:

```sh
sudo apt-get install meson ninja-build g++ pkg-config libwayland-dev wayland-protocols \
    libxkbcommon-dev libegl1-mesa-dev libgles2-mesa-dev libx11-dev nodejs
sudo apt-get install weston         # optional
```

Fedora:

```sh
sudo dnf install meson ninja-build gcc-c++ pkgconf wayland-devel wayland-protocols-devel \
    libxkbcommon-devel mesa-libEGL-devel mesa-libGLES-devel libX11-devel nodejs
sudo dnf install weston             # optional
```

Check with `./check-deps.sh`.

## Build, test, run

```sh
meson setup build --buildtype=debug
meson compile -C build
meson test -C build --print-errorlogs
```

Headless (used by all automated tests; needs no display):

```sh
./build/mansion-desktop --headless --exit-after-ms 2000
```

Windowed, inside your existing desktop session (`DISPLAY` must be set):

```sh
./build/mansion-desktop
./build/mansion-desktop --launch weston-terminal
```

`--help` lists all options. The compositor prints `MANSION_SOCKET=<name>` on
stdout; clients started by hand need `WAYLAND_DISPLAY=<name>` and the same
`XDG_RUNTIME_DIR`.

Protocol debugging: `WAYLAND_DEBUG=1 ./build/mansion-test-client --socket <name>`.

## Workflow

1. Read `docs/STATUS.md` ("Next task") and `docs/TASKS.md`.
2. Do one task. Build. Run `meson test`.
3. Tick the task, update `docs/STATUS.md` and the current `docs/handoffs/NN.md`.
4. Commit: `git add -A && git commit -m "P1-T03: xdg-shell"`.

The conventions (labels for verification, human-only tasks, no test
weakening) are at the top of `docs/TASKS.md`.
