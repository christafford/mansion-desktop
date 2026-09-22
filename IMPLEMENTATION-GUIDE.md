# Implementation Guide

This document provides detailed technical guidance for implementing the projects in the roadmap.

## Project 1: Nested compositor foundation

### Overview
Create a minimal Wayland compositor that can run inside a regular desktop window and launch Wayland clients.

### Step 1: Wayland socket and display

1. Create a Wayland socket (auto or named)
2. Create a `wl_display` and listen on the socket
3. Run the Wayland event loop

Key APIs:
- `wl_display_add_socket()`
- `wl_display_run()`
- `wl_event_loop`

### Step 2: Compositor implementation

Implement the `wl_compositor` interface:
- `create_surface()` - create surface objects
- `create_region()` - create region objects

Implement the `wl_shell` interface (for fullscreen shells):
- `create_shell_surface()` - create shell surface objects
- Handle `shell_surface::ping` - respond to pings
- Handle `shell_surface::move` - start interactive move
- Handle `shell_surface::resize` - start interactive resize
- Handle `shell_surface::set_top_hint` - mark as top-hhost
- Handle `shell_surface::set_maximized` - maximize surface
- Handle `shell_surface::set_fullscreen` - fullscreen surface

### Step 3: EGL rendering

1. Load EGL function pointers
2. Choose EGL config (30bit RGB, no caveats)
3. Create EGL context
4. Create EGL surface for host window (use `EGL_EXT_platform_base` with `EGL_PLATFORM_X11`)

Key APIs:
- `eglGetPlatformDisplayEXT()`
- `eglCreateContext()`
- `eglCreateWindowSurface()`
- `glRenderbuffer()`
- `eglSwapBuffers()`

### Step 4: renderer

Create a simple GLES2 renderer that can:
- Render solid color quads (for solid backgrounds)
- Render textured quads (for client surfaces)
- Output debug text (using a bitmap font)

Shaders needed:
- Vertex shader: transform and projection
- Fragment shader: textured or solid color

### Step 5: surface management

For each `wl_surface`:
- Track role (shell, xdg_shell, etc.)
- Track pending and current states (position, size, transform)
- Store the buffer and damage
- Render with correct transformation

Listen for `wl_surface::commit` and apply state.

### Step 6: Keyboard handling

Implement `wl_keyboard`:
- Create keyboard objects
- Send key events to focused surface
- Send mod器器ifier events
- Send focus enter/leave

Use xkbcommon for:
- Keymap handling
- State tracking
- Keycode to UTF-8 conversion

### Step 7: Pointer handling

Implement `wl_pointer`:
- Create pointer objects
- Send motion events
- Send button events
- Send axis events (for scroll wheels)
- Track cursor surface

### Step 8: Launcher

Create a helper that:
- Sets `WAYLAND_DISPLAY` to the socket name
- Sets `XDG_RUNTIME_DIR` to a temp directory
- Forks and executes the child process
- Handles child death (remove surface)

### Step 9: Client management

- Listen for new clients (`wl_display::get_wl_display()`)
- Assign unique client IDs
- Track clients and clean up when they disconnect

### Minimal test

Launch `weston-terminal`:
- Should display a terminal
- Typing should work
- Resizing should work
- Close button should work
- Killing Mansion Desktop should not crash the host session

### Code structure

```cpp
// src/main.cpp
int main(int argc, char* argv[])
{
    // Parse args (--launch=command)
    // Create display
    // Create compositor
    // Set up EGL
    // Create host window
    // Init renderer
    // Listen for clients
    // Run event loop
}

// src/compositor.cpp
class Compositor {
    // wl_compositor implementation
    // Surface tracking
    // Commit handling
};

// src/display.cpp
class Display {
    // wl_display
    // EGL context
    // Host window
    // Event loop
};

// src/input.cpp
class InputManager {
    // Keyboard and pointer state
    // Seat global
    // Focus handling
};

// src/launch.cpp
class Launcher {
    // Spawn clients
    // Track running clients
};
```

### Testing

1. Build: `meson setup build && meson compile -C build`
2. Run with terminal: `./build/mansion-desktop --launch=weston-terminal`
3. Verify:
   - Terminal appears
   - Input works
   - Resizing works
   - Close works
   - Clean exit

### Tips

- Use `std::unique_ptr` with custom deleters for Wayland objects
- Check for null on all `interface::implementation` calls
- Validate input (e.g., buffer size > 0)
- Handle errors (wl_resource_post_error())
- Implement `WL_DEBUG` env variable for debug prints
