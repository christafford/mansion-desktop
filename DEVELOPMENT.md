# Development Setup

## Native Development Environment

### Arch Linux
```bash
sudo pacman -S wayland-devel libxkbcommon mesa egl wayland-egl meson
```

### Debian/Ubuntu
```bash
sudo apt-get install libwayland-dev libxkbcommon-dev libegl1-mesa-dev mesa-common-dev wayland-protocols meson
```

### Fedora
```bash
sudo dnf install wayland-devel libxkbcommon-devel mesa-libGL-devel wayland-protocols meson
```

## Docker Development Environment

Build the development container:
```bash
docker build -t mansion-desktop-dev -f Dockerfile.dev
```

Run the container:
```bash
docker run -ti --net=host --volume="$PWD:/src" mansion-desktop-dev
```

Inside the container, you can build and run:
```bash
meson setup build --buildtype=debug
meson compile -C build
./build/mansion-desktop --launch=weston-terminal
```

## Building

```bash
meson setup build --buildtype=debug
meson compile -C build
```

Run with a client:
```bash
./build/mansion-desktop --launch=weston-terminal
```

## Testing

Install `weston-terminal` for testing:

On Arch: `sudo pacman -S weston`
On Debian/Ubuntu: `sudo apt-get install weston`

## IDE Setup

### CLion

- Install dependencies as above
n- Open project directory in CLion
- Select meson build configuration
n
### Visual Studio Code

Recommended extensions:
- C/C++ extension
- meson
- Wayland grammar

## Project Structure

- `src/` - C++ source code
- `docs/` - design documents and handoff notes
- `meson.build` - build configuration

## Dependencies

- Wayland - core protocol implementation
- xkbcommon - keyboard input handling
- Mesa - EGL and GLES2 for rendering
- meson - build system
