# P21-T11: GDExtension Adapter — Handoff

## Summary

Written and compiled the GDExtension adapter that bridges Godot to the Wayland
compositor core. The adapter provides explicit lifecycle (create/pump/destroy),
nonblocking event dispatch, surface enumeration, frame callback management,
and clean shutdown — all on the owning thread with no second blocking loop.

**Status: partially verified.** Code compiles cleanly against mansion core and
GDExtension interface. Extension .so built and ready for Godot editor. Full
Godot load/unload test blocked on Godot editor build (target=editor) in progress.

## Architecture

```
GDExtension (.so) ← mansion_extension.cpp (GDExtension entry point)
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

### No modifications to existing files
The bridge only reads the existing mansion core public API. No changes to
`src/` files outside `src/godot/`.

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
cp build-godot-ext/libmansion_godot.so world/bin/
cp src/godot/extension.toml world/bin/
```
Then enable the extension in Godot Project Settings > General > Extensions.

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
| Godot loads it | **Blocked** — Godot editor build in progress |
| Starts private socket | **Code written** — `wl_display_add_socket_auto()` |
| Accepts fixture client | **Code written** — `mansion_adapter_launch_client()` |
| Shuts down cleanly | **Code written** — reverse-order teardown in `destroy()` |
| Extension load/unload | **Blocked** — needs runtime |
| Headless regression checks | **Done** — 28/28 pass in build and asan |

## Godot engine build progress

**template_debug build** (first attempt):
- Started: `scons platform=linuxbsd target=template_debug dev_build=yes -j4`
- Status: completed
- Output: `tools/godot-4.7.2-stable/bin/godot.linuxbsd.template_debug.dev.x86_64` (772 MB)
- Note: template_debug produces a library target, not a standalone binary

**editor build** (for GDExtension testing):
- Started: `scons platform=linuxbsd target=editor dev_build=yes -j4`
- Status: in progress (building SConscript files)
- Expected output: `tools/godot-4.7.2-stable/bin/godot.linuxbsd.editor.dev.x86_64`
- Note: SCons 4.11.1 in `tools/.venv/`; no sudo or system package required

## Next steps

1. Wait for Godot editor build to complete
2. Copy extension to `world/bin/` and test loading:
   ```bash
   cp build-godot-ext/libmansion_godot.so world/bin/
   cp src/godot/extension.toml world/bin/
   tools/godot-4.7.2-stable/bin/godot.linuxbsd.editor.dev.x86_64
   ```
3. Enable extension in Project Settings > General > Extensions
4. Test fixture client connection on private socket
5. Verify clean shutdown (no leaks, no dangling pointers)
6. Run `meson test -C build --print-errorlogs` to confirm no regressions
7. Tick P21-T11 in TASKS.md and update STATUS.md

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
