# Project Status

## Current State

- Repository created with concept and roadmap
- Project 1 (nested compositor foundation) - partially implemented
- Development environment setup documented
- Docker development environment defined
- Build system (Meson) configured
- Wayland display and global protocols set up
- Compositor and shell interfaces created (stubs)

## Next Steps

### Option 1: Set up native development environment

Install required dependencies:

On Arch Linux:
```bash
sudo pacman -S wayland-devel libxkbcommon mesa egl wayland-egl meson
```

On Debian/Ubuntu:
```bash
sudo apt get install libwayland-dev libxkbcommon-dev libegl1-mesa-dev mesa-common-dev wayland-protocols meson
```

Then build and test:
```bash
meson setup build --buildtype=debug
meson compile -C build
./build/mansion-desktop --launch=weston-terminal
```

### Option 2: Use Docker development environment

Build the container:
```bash
docker build -t mansion-desktop-dev -f Dockerfile.dev
```

Run the container:
```bash
docker run -ti --net=host --volume="$PWD:/src" mansion-desktop-dev
```

Inside container, build and test:
```bash
meson setup build --buildtype=debug
meson compile -C build
./build/mansion-desktop --launch=weston-terminal
```

### Option 3: Start with a different language

If Rust is made available, implement Project 1 using Rust and Smithay (as originally suggested in the roadmap).

## Once dependencies are available

1. Implement EGL rendering (display.cpp)
2. Implement input device handling (input.cpp)
3. Implement surface commit handling (compositor.cpp)
4. Implement client launcher (launch.cpp)
5. Verify weston-terminal works

Files to create/complete:
- `meson.build` (configured)
- `src/main.cpp` (partial)
- `src/compositor.cpp` (partial)
- `src/shell.cpp` (partial)
- `src/display.cpp` (empty)
- `src/input.cpp` (empty)
- `src/launch.cpp` (empty)

Test commands:
```bash
meson setup build --buildtype=debug
meson compile -C build
./build/mansion-desktop --launch=weston-terminal
```

## Documentation

- [mansion-desktop-concept.md](mansion-desktop-concept.md) - high-level design
- [PROJECT-ROADMAP.md](PROJECT-ROADMAP.md) - implementation plan
- [IMPLEMENTATION-GUIDE.md](IMPLEMENTATION-GUIDE.md) - technical details for Project 1
- [DEVELOPMENT.md](DEVELOPMENT.md) - setup instructions
- [docs/handoffs/01.md](docs/handoffs/01.md) - detailed task list
- [docs/decisions/01-md](docs/decisions/01-md) - technology stack

## Next Steps

### Option 1: Set up native development environment

Install required dependencies:

On Arch Linux:
```bash
sudo pacman -S wayland-devel libxkbcommon mesa egl wayland-egl meson
```

On Debian/Ubuntu:
```bash
sudo apt get install libwayland-dev libxkbcommon-dev libegl1-mesa-dev mesa-common-dev wayland-protocols meson
```

Then build and test:
```bash
meson setup build --buildtype=debug
meson compile -C build
./build/mansion-desktop --launch=weston-terminal
```

### Option 2: Use Docker development environment

Build the container:
```bash
docker build -t mansion-desktop-dev -f Dockerfile.dev
```

Run the container:
```bash
docker run -ti --net=host --volume="$PWD:/src" mansion-desktop-dev
```

Inside container, build and test:
```bash
meson setup build --buildtype=debug
meson compile -C build
./build/mansion-desktop --launch=weston-terminal
```

### Option 3: Start with a different language

If Rust is made available, implement Project 1 using Rust and Smithay (as originally suggested in the roadmap).

## Once dependencies are available

1. Implement a minimal Wayland compositor (see IMPLEMENTATION-GUIDE.md)
2. Create a launcher helper
3. Verify weston-terminal works

Files to create:
- `meson.build` (configured)
- `src/main.cpp` (placeholder exists)
- `src/compositor.cpp` - Core compositor logic
- `src/display.cpp` - Display/EGL handling
- `src/input.cpp` - Input device handling
- `src/launch.cpp` - Client launcher

Test commands:
```bash
meson setup build --build-type=debug
meson compile -C build
./build/mansion-desktop --launch=weston-terminal
```

## Documentation

- [mansion-desktop-concept.md](mansion-desktop-concept.md) - high-level design
- [PROJECT-ROADMAP.md](PROJECT-ROADMAP.md) - implementation plan
- [IMPLEMENTATION-GUIDE.md](IMPLEMENTATION-GUIDE.md) - technical details for Project 1
- [DEVELOPMENT.md](DEVELOPMENT.md) - setup instructions
- [docs/handoffs/01.md](docs/handoffs/01.md) - detailed task list
- [docs/decisions/01-md](docs/decisions/01-md) - technology stack

