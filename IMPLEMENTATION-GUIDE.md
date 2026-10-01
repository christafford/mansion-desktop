# Implementation guide

Technical reference for the tasks in [docs/TASKS.md](docs/TASKS.md). The
roadmap says *what* and *when*; the decisions in `docs/decisions/` say *why*;
this file collects the *how* that keeps recurring. Keep it short and correct;
delete anything that stops being true.

## Ground rules

- Supplied implementation: libwayland-server, C++20, Meson and EGL/GLES2.
  Decision 06 now authorizes a Godot frontend/GDExtension bridge with the retained
  C++ core. See GODOT-INTEGRATION.md for its target contracts; the legacy GL
  examples below are not instructions to keep rebuilding the custom scene.
- ARCHITECTURE.md defines boundaries; Project 21 in TASKS.md is the active work
  order. Art and core extraction are independently eligible; P21-T19 is their
  furnished live-terminal convergence gate.
- Protocol: `wl_compositor`, `wl_shm`, `wl_seat`, `xdg_wm_base` (+ popups
  later), `wl_output` when chosen clients require it. `wl_shell` was removed.
  Pin supported versions to implemented behavior, not generated-header versions.
- The compositor is nested. Input comes from the host window, never from
  `/dev/input`.
- Test logic/protocol ownership headless where practical. Engine runtime, visual
  quality and real-client interaction require actual corresponding evidence.
  Label agent GUI observations; only a person can tick final human acceptance.

## libwayland-server patterns

```cpp
// Global
wl_global_create(display, &xdg_wm_base_interface, 6, data, bind_fn);

// Bind: create the resource at the version the client asked for (capped by ours)
auto* res = wl_resource_create(client, &xdg_wm_base_interface, std::min(version, 6u), id);
wl_resource_set_implementation(res, &impl, data, destroy_fn);

// Request table layout must match the generated interface for the pinned XML.
// Implement advertised-version requests; do not use nullptr to imply support.
static const struct xdg_wm_base_interface impl = { destroy, create_positioner, get_xdg_surface, pong };

// Resource user data
auto* s = static_cast<Surface*>(wl_resource_get_user_data(res));

// Errors
wl_resource_post_error(res, XDG_WM_BASE_ERROR_ROLE, "surface already has a role");

// Watching another resource's destruction (for example a wl_buffer)
struct wl_listener l; l.notify = on_buffer_destroy; wl_resource_add_destroy_listener(buffer, &l);
```

Rules of thumb:

- Listener and interface structs must be static or owned for the resource's
  life. Stack copies corrupt memory.
- Never keep a raw `wl_resource*` past its destroy callback; null it there.
- Serials: one monotonically increasing `uint32_t` per compositor
  (`wl_display_next_serial(display)`).
- Time stamps for input and frame callbacks: milliseconds from
  `CLOCK_MONOTONIC`, truncated to 32 bits.
- A client sees seat events on the roundtrip *after* the bind.

## xdg-shell handshake (P1-T03)

1. Client: `get_xdg_surface(wl_surface)`, `get_toplevel`, sets title/app_id,
   `wl_surface.commit` (no buffer).
2. Server: `xdg_toplevel.send_configure(w, h, states)`,
   `xdg_surface.send_configure(serial)`.
3. Client: `ack_configure(serial)`, attaches a buffer, commits.
4. Server: surface is mapped; send keyboard `enter` if it gets focus.

`xdg_wm_base.ping` every few seconds is optional; if sent, expect `pong` or
treat the client as unresponsive.

## Buffers and rendering (P1-T04, P1-T05)

- `wl_shm_buffer* shm = wl_shm_buffer_get(buffer_resource)`; null means it is
  not an shm buffer (dmabuf/EGL later).
- Formats to support first: `WL_SHM_FORMAT_ARGB8888`, `XRGB8888` (little
  endian BGRA in memory). Upload with `GL_BGRA_EXT` when
  `GL_EXT_texture_format_BGRA8888` is available, else swizzle.
- attach changes pending state; commit applies it. No new attach preserves
  content; attach-null removes content only on the next commit.
- Release a committed buffer when the compositor has finished using its storage,
  not simply when another buffer is attached. For copied shm pixels this may be
  after the copy/upload; imported GPU buffers need a defined synchronization
  strategy. Never access a client's reused storage after release. A replaced,
  uncommitted pending attach is not a consumed buffer.
- Track buffer resource destruction separately from owned pixel/texture content.
  Use begin/end shm access around reads, respect stride and declared format,
  handle premultiplied alpha, and test unequal RGB values. Do not choose formats
  using a global flag or undocumented driver swizzles.
- EGL extension lookup uses eglGetProcAddress plus relevant extension checks.
  Verify imports with actual buffers and errors; export support or PBuffer
  readback does not prove Wayland GPU-client import.
- Frame callbacks: send `done` once per rendered frame for surfaces that were
  visible, then destroy the callback resource.
- Projection: pixels to clip space `x' = 2x/W - 1`, `y' = 1 - 2y/H`.
- Headless GL: `EGL_PLATFORM_SURFACELESS_MESA` + pbuffer; `glReadPixels` into a
  P6 PPM for `--screenshot`.

## Input (P1-T06, P1-T07)

- Keycodes on the wire are evdev codes; X11 keycode − 8 = evdev code.
- `xkb_keymap_new_from_names(ctx, &names, 0)` with `names` from
  `XKB_DEFAULT_RULES/MODEL/LAYOUT/VARIANT/OPTIONS`; send the keymap string via
  a `memfd` and `wl_keyboard.keymap`.
- Track modifier state with `xkb_state_update_key`, send
  `wl_keyboard.modifiers` when it changes and on `enter`.
- Pointer: `enter(serial, surface, sx, sy)` when the pointer crosses into a
  surface, `leave` before entering another, `motion` in surface-local
  coordinates (`wl_fixed_from_double`), `button` with `BTN_*` codes, `frame`
  after each group for pointer v5+.
- On leaving application mode or losing focus: release held keys/buttons,
  reset modifiers, and send the appropriate keyboard/pointer leave events.
  Verify full-size letterboxing, logical coordinates, scale and transforms.
- xdg_toplevel.close is a request to the application; it may be declined.
  Compositor close controls must not forcibly destroy protocol resources.
- Popups require positioners, configure/ack/commit, valid grab serials/parents,
  surface-tree rendering and correct input/dismissal. A configure-only test
  does not establish working menus.

## Launcher (P1-T08)

Set `WAYLAND_DISPLAY`, unset `DISPLAY`, inherit `XDG_RUNTIME_DIR`; reap with
`SIGCHLD` via a self-pipe or `signalfd` added to the Wayland event loop
(`wl_event_loop_add_fd`).

## Tests

See [tests/README.md](tests/README.md). New behaviour → new script → new
`test()` line. Never assert on timing you can assert on output.

## Protocol references

Use the XML installed for the pinned wayland/wayland-protocols versions as the
implementation contract. Public references help explain behavior; newer protocol
features are not implied support for this prototype.

- [Wayland core specification](https://wayland.freedesktop.org/docs/html/apa.html)
- [Official xdg-shell XML](https://gitlab.freedesktop.org/wayland/wayland-protocols/-/blob/main/stable/xdg-shell/xdg-shell.xml)
- [Khronos EGL extension lookup](https://registry.khronos.org/EGL/sdk/docs/man/html/eglGetProcAddress.xhtml)
