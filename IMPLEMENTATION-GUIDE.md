# Implementation guide

Technical reference for the tasks in [docs/TASKS.md](docs/TASKS.md). The
roadmap says *what* and *when*; the decisions in `docs/decisions/` say *why*;
this file collects the *how* that keeps recurring. Keep it short and correct;
delete anything that stops being true.

## Ground rules

- libwayland-server directly, C++20, Meson. No wlroots (see decision 02).
- Protocol: `wl_compositor`, `wl_shm`, `wl_seat`, `xdg_wm_base` (+ popups
  later), `wl_output` when clients need it. `wl_shell` is deprecated and goes
  away in P1-T03.
- The compositor is nested. Input comes from the host window, never from
  `/dev/input`.
- Everything testable is tested headless. A person only confirms feel and real
  applications.

## libwayland-server patterns

```cpp
// Global
wl_global_create(display, &xdg_wm_base_interface, 6, data, bind_fn);

// Bind: create the resource at the version the client asked for (capped by ours)
auto* res = wl_resource_create(client, &xdg_wm_base_interface, std::min(version, 6u), id);
wl_resource_set_implementation(res, &impl, data, destroy_fn);

// Requests table: order and count must match the XML exactly; use nullptr for unhandled optional ones.
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
- Release the *previous* buffer (`wl_buffer_send_release`) after the new one is
  committed and uploaded. Clients reuse released buffers.
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
- On leaving application mode or losing focus: release every held key, send
  `modifiers`, then `leave`.

## Launcher (P1-T08)

Set `WAYLAND_DISPLAY`, unset `DISPLAY`, inherit `XDG_RUNTIME_DIR`; reap with
`SIGCHLD` via a self-pipe or `signalfd` added to the Wayland event loop
(`wl_event_loop_add_fd`).

## Tests

See [tests/README.md](tests/README.md). New behaviour → new script → new
`test()` line. Never assert on timing you can assert on output.
