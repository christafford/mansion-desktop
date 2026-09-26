# Project Status

## Current State

### Completed
- **Build system**: Meson configured, compiles with zero critical warnings
- **Wayland display**: Socket creation, event loop, client connection management
- **Globals**: `wl_compositor` (v4), `wl_shell` (v1), `wl_seat` (v4), `wl_shm` (via `wl_display_init_shm`)
- **Client launcher**: `fork/exec` with proper child process cleanup (SIGTERM→wait→SIGKILL)
- **EGL rendering**: X11+EGL platform, GLES2 context, Mesa3D shaders, frame rendering
- **Surface management**: `wl_surface` commit handling, pending/current position tracking, dimension tracking
- **Input handling**: evdev scanning (`/dev/input/`), 14 devices detected (keyboards + mice), event reading in non-blocking mode
- **Seat**: Keyboard + pointer capabilities advertised, xkb keymap sent via memfd

### Verified
- Build compiles with `ninja -C build`
- Compositor starts and binds socket
- All 4 globals advertised
- Client connection and protocol negotiation works
- Seat events (name + capabilities) delivered to clients on second roundtrip
- 14 input devices detected (Keychron Q10, USB Keyboard, Steam Controller, etc.)
- Input events forwarded to Wayland clients (keyboard key events, pointer motion/buttons)

### Not Yet Implemented
- Surface buffer rendering (currently renders solid color placeholders)
- Surface enter/leave events to clients
- Focused surface tracking
- Shell surface operations (ping/pong, move, resize, fullscreen)
- wl_keyboard focus management (enter/leave events to clients)
- wl_pointer enter/leave events (needs surface-under-cursor tracking)

## Next Steps

### 1. Surface Enter Events
- Send `wl_surface::enter` events when a surface is created/comitted
- Track output association for multi-monitor setups

### 2. Keyboard Focus
- Send `wl_keyboard::enter` event when surface gets focus
- Track which surface has keyboard focus
- Send `wl_keyboard::leave` when focus changes

### 3. Pointer Enter/Leave
- Track surface under pointer coordinates
- Send `wl_pointer::enter` when pointer moves onto a surface
- Send `wl_pointer::leave` when pointer moves off

### 4. Surface Rendering
- Implement actual buffer rendering (textured quads from wl_shm buffers)
- Handle buffer damage regions
- Support surface stacking/z-order

### 5. Shell Surface Operations
- Handle `wl_shell_surface::ping` (send pong)
- Implement `set_toplevel`, `set_popup`, `setfullscreen`
- Implement `move` and `resize` grab support

## Test Commands

```bash
meson compile -C build
./build/mansion-desktop

# Test client connects and receives seat events
env WAYLAND_DISPLAY=mansion-desktop ./test_minimal

# With a terminal (install foot or similar first)
./build/mansion-desktop --launch=foot
```

## Architecture Notes

### Wayland Roundtrip Behavior
Clients need **two roundtrips** to receive seat events:
1. First roundtrip: get globals, bind to them (seat resource created, events queued)
2. Second roundtrip: receive seat events (name, capabilities)

This is because `wl_registry_bind()` sends a request synchronously, but the server sends seat events asynchronously after processing the bind request.

### wl_seat_listener
The listener struct must be a static/global variable. Inline stack allocation can cause memory corruption when libwayland copies the struct.

## Known Issues

1. **No surface rendering**: Surfaces show as solid color placeholders, not actual buffer content
2. **No input focus**: Keyboard events are read from evdev but not associated with any focused surface
3. **No pointer surface tracking**: Pointer coordinates tracked but no enter/leave events sent
4. **Dead keyboard devices**: Power buttons detected as keyboards (filter by KEY_ENTER presence)

## Files

- `src/main.cpp` - Main loop, signal handling, event dispatch
- `src/compositor.cpp` - wl_compositor, surface management
- `src/compositor-private.h` - MansionSurface struct, internal shared definitions
- `src/display.cpp` - EGL/X11 rendering, Mesa shaders
- `src/input.cpp` - evdev handling, seat creation, event forwarding
- `src/shell.cpp` - wl_shell, wl_shell_surface
- `src/launch.cpp` - Client launcher helper
- `docs/STATUS.md` - This file
- `docs/handoffs/01.md` - Handoff document
- `PROJECT-ROADMAP.md` - Full implementation plan
