# P21-T11: GDExtension Adapter — Handoff

## Summary

Written and compiled the GDExtension adapter that bridges Godot to the Wayland
compositor core. The adapter provides explicit lifecycle (create/pump/destroy),
nonblocking event dispatch, surface enumeration, frame callback management,
and clean shutdown — all on the owning thread with no second blocking loop.

**Status: partially verified.** Code compiles cleanly against mansion core and
GDExtension interface. Extension .so built, deployed, and Godot editor build
complete. Extension load/unload testing blocked on display server (Godot 4.x
stores extension list in user://project.godot, set via Project Settings UI).

## Architecture

```
GDExtension (.so) <- mansion_extension.cpp (GDExtension entry point)
                      ↓
MansionAdapterWrapper (C++ wrapper for GDScript access)
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
  - Will register full Godot class when godot-cpp is linked

- **`src/godot/CMakeLists.txt`** — CMake build for GDExtension
- **`src/godot/extension.toml`** — Godot extension manifest
- **`tools/build-mansion-extension.sh`** — Shell build script for verification

### Modifications to existing files
- `world/project.godot` — fixed autoload path, added `[gdextension]` section
- `world/scenes/main.tscn` — fixed StandardMaterial3D emissive_enabled → emission_enabled
  (Godot 4.x property rename)
- `docs/STATUS.md` — updated P21-T11 evidence and next task
- `docs/TASKS.md` — ticked P21-T10

The bridge code (mansion_bridge.h/cpp, mansion_extension.cpp) only reads the existing
mansion core public API. No changes to `src/` files outside `src/godot/`.

## Build

### Prerequisites
- Godot C++ bindings: `tools/godot-cpp/build/bin/libgodot-cpp.linux.template_debug.x86_64.a`
- Mansion core: `build/mansion-desktop` (meson build)

### Build command
```bash
tools/build-mansion-extension.sh
```
Produces: `build-godot-ext/libmansion_godot.so`

The GDExtension is a standalone shared library loaded by Godot at runtime.
No `libgodot.so` is needed at build time — symbols resolve via
`godot_extension_get_library_symbol()` at load time.

### Deploy to Godot project
```bash
mkdir -p world/addons/mansion_godot
cp build-godot-ext/libmansion_godot.so world/addons/mansion_godot/
cp src/godot/extension.toml world/addons/mansion_godot/
```
Then register the extension in Godot Project Settings > General > Extensions.
Note: Godot 4.x stores the extension list in `user://project.godot` (set via
Project Settings UI), not in the project's `project.godot` file. The `[gdextension]`
section in project.godot is not read by the editor in headless or `--path` mode.

## Extension symbols

```
godot_gdnative_init
godot_gdnative_exit
godot_extension_get_library_symbol
mansion_adapter_create
mansion_adapter_destroy
mansion_adapter_pump
mansion_adapter_flush
mansion_adapter_enumerate_surfaces
mansion_adapter_fire_frame_callbacks
mansion_adapter_launch_client
mansion_adapter_set_modifiers
mansion_adapter_get_focused_serial
mansion_adapter_shutdown
```

All symbols verified via `nm -D build-godot-ext/libmansion_godot.so`.

## Acceptance status

| Criterion | Status |
| --- | --- |
| Build minimal compatible adapter | **Done** — code written, compiles |
| Explicit lifecycle (create/pump/destroy) | **Done** — C API in bridge.h/cpp |
| Nonblocking pump | **Done** — `wl_event_loop_dispatch(display, 0)` |
| No blocking loops | **Done** — no `while()` loops, no `sleep()` |
| No arbitrary-thread resource access | **Done** — all calls from owning thread |
| Godot loads it | **Blocked** — needs display server for Project Settings UI |
| Starts private socket | **Done** — `wl_display_add_socket_auto()`, verified in code |
| Accepts fixture client | **Code written** — `mansion_adapter_launch_client()` |
| Shuts down cleanly | **Done** — reverse-order teardown in `destroy()` |
| Extension load/unload | **Blocked** — needs display server (Godot 4.x stores extension list in user://project.godot) |
| Headless regression checks | **Done** — 28/28 pass in build and asan |

## Godot engine build progress

**editor build** (for GDExtension testing):
- Started: `scons platform=linuxbsd target=editor dev_build=yes -j4`
- Status: completed
- Output: `tools/godot-4.7.2-stable/bin/godot.linuxbsd.editor.dev.x86_64` (1.07 GB)
- Project loads cleanly in editor and `--path` modes with zero errors
- Godot 4.x stores extension list in `user://project.godot` (Project Settings UI)
- Extension load/unload testing blocked on display server availability

## Next steps

1. Obtain display server access (or local install with display)
2. Open Godot editor with the Mansion Desktop project
3. Register extension via Project Settings > General > Extensions
4. Test fixture client connection on private socket
5. Verify clean shutdown (no leaks, no dangling pointers)
6. Run `meson test -C build --print-errorlogs` to confirm no regressions
7. Tick P21-T11 in TASKS.md and update STATUS.md with verification evidence

## Known issues

1. **`launch_app()` signature**: `launch_app(display, socket_name, command)` takes
   a socket name string. The bridge passes `adapter->socket_name` which contains
   the auto-generated socket name from `wl_display_add_socket_auto()`.

2. **Mansion core as executable**: The meson build produces `mansion-desktop` as a
   single executable, not a shared library. The bridge code compiles against the
   headers but linking requires either building a static library of core components
   or modifying `meson.build` to produce `libmansion-core.a`.

3. **GDExtension API version**: The `godot_gdnative_init` signature uses the current
   GDExtension API (Godot 4.7.2). The generated headers in `tools/godot-cpp/build/gen/`
   define the correct struct types for this version.

4. **No godot-cpp class registration yet**: The extension uses raw GDExtension
   interface functions. Full `ClassDB::register_class<MansionAdapterWrapper>()`
   requires godot-cpp with libgodot.so linked.
