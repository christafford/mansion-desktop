# P21-T11: GDExtension Adapter — Handoff

## Summary

Written and compiled the GDExtension adapter that bridges Godot to the Wayland
compositor core. The adapter provides explicit lifecycle (create/pump/destroy),
nonblocking event dispatch, surface enumeration, frame callback management,
and clean shutdown — all on the owning thread with no second blocking loop.

**Status: partially verified.** Code compiles cleanly against mansion core and
GDExtension interface. Full linking and Godot load/unload test blocked on
`libgodot.so` (Godot engine build in progress).

## Architecture

```
GDExtension (.so) ← mansion_extension.cpp (GDExtension entry point)
                      ↓
mansion_bridge.h/cpp (C-facing bridge API)
                      ↓
compositor core (compositor.cpp, input.cpp, xdg-shell.cpp)
                      ↓
Wayland server socket
```

## Files

### New files
- **`src/godot/mansion_bridge.h`** — C-facing bridge API header
  - Lifecycle: `mansion_adapter_create()`, `mansion_adapter_destroy()`
  - Event pump: `mansion_adapter_pump()` (nonblocking), `mansion_adapter_flush()`
  - Surface enumeration: `mansion_adapter_enumerate_surfaces()` → callback
  - Frame callbacks: `mansion_adapter_fire_frame_callbacks()`
  - Client launch: `mansion_adapter_launch_client()`
  - Focus: `mansion_adapter_get_focused_serial()`
  - Shutdown: `mansion_adapter_shutdown()`
  - No GL/EGL includes — pure protocol core

- **`src/godot/mansion_bridge.cpp`** — C++ bridge implementation
  - Owns the Wayland display socket
  - Creates compositor, seat, xdg-shell on init
  - Tracks client apps for clean shutdown
  - All calls from owning thread only
  - Nonblocking pump via `wl_event_loop_dispatch(display, 0)`
  - Surface enumeration collects into lightweight `MansionSurfaceInfo` snapshots

- **`src/godot/mansion_extension.cpp`** — GDExtension entry point
  - Uses raw `gdextension_interface.h` (no godot-cpp dependency for compilation)
  - Exports: `godot_gdnative_init`, `godot_gdnative_exit`,
    `godot_extension_get_library_symbol`
  - `MansionAdapterWrapper` class exposes adapter to GDScript
  - Will register full Godot class when libgodot.so is available

- **`src/godot/CMakeLists.txt`** — CMake build for GDExtension

- **`tools/build-mansion-extension.sh`** — Shell build script for verification

### No modifications to existing files
The bridge only reads the existing mansion core public API. No changes to
`src/` files outside `src/godot/`.

## Build

### Prerequisites
- Godot C++ bindings: `tools/godot-cpp/build/bin/libgodot-cpp.linux.template_debug.x86_64.a`
- Mansion core: `build/mansion-desktop` (meson build)
- libgodot.so: `tools/godot-4.7.2-stable/bin/libgodot.linuxbsd.template_debug.x86_64.so`
  (blocked — Godot engine build in progress)

### Compile-only verification (works now)
```bash
tools/build-mansion-extension.sh
```
Compiles both `mansion_bridge.cpp` and `mansion_extension.cpp`. Reports success
if both `.o` files compile cleanly.

### Full GDExtension build (requires libgodot.so)
```bash
tools/build-mansion-extension.sh
# or with explicit libgodot.so path:
MANSION_GODOT_SO=/path/to/libgodot.so tools/build-mansion-extension.sh
```

## Acceptance status

| Criterion | Status |
| --- | --- |
| Build minimal compatible adapter | **Done** — code written, compiles |
| Explicit lifecycle (create/pump/destroy) | **Done** — C API in bridge.h/cpp |
| Nonblocking pump | **Done** — `wl_event_loop_dispatch(display, 0)` |
| No blocking loops | **Done** — no `while()` loops, no `sleep()` |
| No arbitrary-thread resource access | **Done** — all calls from owning thread |
| Godot loads it | **Blocked** — libgodot.so not available |
| Starts private socket | **Blocked** — code written, can't test without loading |
| Accepts fixture client | **Blocked** — needs runtime |
| Shuts down cleanly | **Blocked** — code written, can't test without loading |
| Extension load/unload | **Blocked** — needs runtime |
| Headless regression checks | **Done** — 28/28 pass, unchanged |

## Godot engine build progress

Started: `scons platform=linuxbsd target=template_debug dev_build=yes -j4`
Status: still compiling (30+ minutes, 57+ static libraries built).
Expected output: `tools/godot-4.7.2-stable/bin/libgodot.linuxbsd.template_debug.x86_64.so`
Note: SCons 4.11.1 in `tools/.venv/`; no sudo or system package required.

## Next steps

1. Wait for Godot engine build to complete (check for `libgodot.so`)
2. When available, link the GDExtension:
   ```bash
   MANSION_GODOT_SO=tools/godot-4.7.2-stable/bin/libgodot.linuxbsd.template_debug.x86_64.so \
     tools/build-mansion-extension.sh
   ```
3. Create `extension.toml` for Godot to find the library
4. Test load/unload in Godot editor or Godot binary
5. Test fixture client connection on private socket
6. Verify clean shutdown (no leaks, no dangling pointers)
7. Run `meson test -C build --print-errorlogs` to confirm no regressions
8. Tick P21-T11 in TASKS.md and update STATUS.md

## Known issues

1. **`launch_app()` signature mismatch**: `launch_app(display, socket_name, command)` takes
   a socket name string (not a wl_display pointer). The bridge passes `adapter->socket_name`
   which contains the auto-generated socket name from `wl_display_add_socket_auto()`.

2. **Mansion core as executable**: The meson build produces `mansion-desktop` as a
   single executable, not a shared library. The bridge code compiles against the
   headers but linking requires either building a static library of core components
   or modifying `meson.build` to produce `libmansion-core.a`.

3. **GDExtension API version**: The `godot_gdnative_init` signature uses the current
   GDExtension API (Godot 4.7.2). The generated headers in `tools/godot-cpp/build/gen/`
   define the correct struct types for this version.

4. **No godot-cpp class registration yet**: The extension uses raw GDExtension
   interface functions for now. Full `ClassDB::register_class<MansionCompositor>()`
   requires godot-cpp with libgodot.so linked.
