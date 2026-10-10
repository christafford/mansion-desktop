# Getting started

## Current Godot/Poly Haven direction

Project 21 and Decision 06 are the current implementation plan. The commands
below build the supplied C++ prototype; they do not launch the new engine yet.
P21-T00 pins repository-local Godot/godot-cpp/Blender tools in TOOLCHAIN.md and
P21-T01/T11 add the actual frontend/extension commands. Preserve the existing
core tests. Do not install host packages from an unattended agent; the system
package examples below are for the owner. Official pinned repository-local
tools/assets are permitted by Decision 06.

For an autonomous Qwen run, apply the patch series and use
[OPENCODE-AUTOCONTINUE.md](docs/OPENCODE-AUTOCONTINUE.md). Its Project 21 launch
command sets the scope; zero count/time budgets mean no fixed run deadline.


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
./build/elsewhere --headless --exit-after-ms 2000
```

Windowed, inside your existing desktop session (`DISPLAY` must be set):

```sh
./build/elsewhere
./build/elsewhere --launch weston-terminal
```

`--help` lists all options. The compositor prints `ELSEWHERE_SOCKET=<name>` on
stdout; clients started by hand need `WAYLAND_DISPLAY=<name>` and the same
`XDG_RUNTIME_DIR`.

Protocol debugging: `WAYLAND_DEBUG=1 ./build/elsewhere-test-client --socket <name>`.

## Workflow

1. Read AGENTS.md, STATUS/TASKS, ARCHITECTURE and Decision 06. Start at P21-T00.
2. Take one eligible task within the authorized scope. Verify its acceptance;
   use normal/sanitizer suites for code, and real-client/human trials where stated.
3. Tick only satisfied tasks. Update STATUS and the applicable handoff with
   date/revision/environment, commands, results, limits and next eligible task.
4. Stage only task files (`git add -- <task files>`), then commit locally with
   `<task id>: <summary>`. Never include unrelated user work or push automatically.

Use docs/ACCEPTANCE.md for the current flat/room terminal trial. The native host
backend investigation may change commands later; do not invent unsupported flags.

The conventions (labels for verification, human-only tasks, no test
weakening) are at the top of `docs/TASKS.md`.
