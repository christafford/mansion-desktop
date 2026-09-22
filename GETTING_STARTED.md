# Getting Started with Mansion Desktop Development

## Quick Start

### Using Docker (recommended)

1. Build the development container:
```bash
docker build -t mansion-desktop-dev -f Dockerfile.dev
```

2. Run the container:
```bash
docker run -ti --net=host --volume="$PWD:/src" mansion-desktop-dev
```

3. Inside the container, build and test:
```bash
meson setup build --buildtype=debug
meson compile -C build
./build/mansion-desktop --launch=weston-terminal
```

### Native Setup

Install dependencies:

**Arch Linux:**
```bash
sudo pacman -S wayland-devel libxkbcommon mesa egl wayland-egl meson weston
```

**Debian/Ubuntu:**
```bash
sudo apt-get install libwayland-dev libxkbcommon-dev libegl1-mesa-dev mesa-common-dev wayland-protocols meson weston
```

Then build:
```bash
meson setup build --buildtype=debug
meson compile -C build
```

## Project Structure

- `src/` - C++ source code (C++20)
- `docs/` - design documents and handoff notes
- `meson.build` - build configuration

## Development Workflow

1. **Pick a project** from [PROJECT-ROADMAP.md](PROJECT-ROADMAP.md)

2. **Read the handoff** in `docs/handoffs/[NN].md`

3. **Implement** the task

4. **Test** both automated checks and manual demonstration

5. **Update documentation**
- `docs/STATUS.md` - overall progress
- `docs/handoffs/[NN].md` - next steps
- `docs/decisions/[NN]-*.md` - major decisions

## Running Mansion Desktop

```bash
./build/mansion-desktop --launch=<command>
```

Example:
```bash
./build/mansion-desktop --launch=weston-terminal
```

## Debugging

Run with `WL_DEBUG=1` for Wayland protocol logging:
```bash
WL_DEBUG=1 ./build/mansion-desktop --launch=weston-terminal
```

Use `meson test` to run automated checks (once added).

## Useful Resources

- [Wayland book](https://wayland.book.org/)
- [wlroots](https://gitlab.freedesktop.org/wlroots/wlroots) - modular Wayland compositor library
- [Weston compositor code](https://gitlab.freedesktop.org/wayland/weston)
- [Wayland API reference](https://wayland.book.org/libwayland.html)
- [Smithay (Rust)] (https://smithay.github.io/) - conceptual reference

## Current Status

This is the initial phase of the project. The first milestone is a nested compositor that can display a single live Wayland application and switch between world and application modes.

See [docs/STATUS.md](docs/STATUS.md) for current progress.
