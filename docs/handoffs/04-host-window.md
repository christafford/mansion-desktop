# Handoff 04-host-window — Wayland Client Window + Seat Input (2026-09-28)

## P4-T29 host-navigation repair (2026-10-02)

Follow-up against `a5ba64a`: the user reported a rendered but unresponsive view.
An agent-run four-second launch exited after 120 frames, distinguishing a
static camera from the prior event-loop deadlock. Source inspection found:

- `live_w/a/s/d` and mouse deltas had no consumer; only input scripts moved
  the camera. The main loop now applies host state using frame elapsed time.
- The key handler used 26/38/39/40 instead of Linux `KEY_W/A/S/D` and reset all
  held keys on every event. It now updates only the affected movement key.
- Keyboard leave did not clear held movement. Focus and mode changes now clear
  state; application mode does not receive world camera actions.
- Ordinary pointer motion was ignored; it now supplies deltas if the host lacks
  relative-pointer support. Relative motion retains fractional values.
- Seat listeners were installed after the startup roundtrips; they now register
  at bind time, with capability-based device creation/removal. The xdg ping
  callback existed but was never registered; it now answers pings.
- Removed global evdev scanning/forwarding so background device activity cannot
  duplicate or bypass focused host input. No device permissions were changed.

**Verified by automated test:** `meson compile -C build` and
`meson test -C build --print-errorlogs`: 30/30 pass. `host-input` calls the actual
private host listeners and asserts camera translation, simultaneous keys,
independent releases, ordinary/fractional relative mouse look, one-time delta
consumption, focus loss/re-entry, and application-mode isolation.

Agent-run command:
`WAYLAND_DEBUG=1 timeout -k 2s 15s build/mansion-desktop --room-camera --exit-after-ms 8000`.
It exits 0 after 274 frames; the host sends seat capabilities and keyboard
enter, and ping serial 5443 receives pong 5443. This proves host protocol
progress, not physical-input or visual usability.

**Not verified:** human keyboard/mouse usability, application typing/clicks,
unlimited mouse look (no pointer lock), resizing, real terminal acceptance.
For manual navigation: launch `build/mansion-desktop --room-camera`, focus its
window, hold WASD and move the pointer inside it; releasing keys stops movement,
and switching to another window clears held navigation. Do not tick P4-T12 or
human tasks from the synthetic tests. Continue Project 21 per STATUS.md.

## P4-T19 startup-hang repair (2026-10-02)

Agent-run evidence against base revision `04ecfe6` plus this repair:

- `timeout -k 2s 8s build/mansion-desktop --room-camera --exit-after-ms 1000`
  before the fix completed EGL initialization but logged no rendered frames;
  the timeout had to SIGKILL it (exit 137). After rebuilding, the identical
  command rendered 18 frames and exited 0. These are process/log observations,
  not visual or human usability acceptance.
- Root cause: `client_display_fd_handler` called `wl_display_read_events`
  without `wl_display_prepare_read`. Libwayland waits indefinitely in this
  invalid read sequence, blocking the single main thread and its exit timer.
  Use `wl_display_dispatch` on the readable connection; it handles the read
  preparation internally. Check `wl_display_get_error` in the main loop so a
  disconnected host terminates with an error instead of spinning.
- `tests/host_events.cpp` drives the actual handler through a Wayland event
  loop with a socket-pair server/client, three sync replies, and disconnect.
  Compiled with the original handler, its three-second alarm terminates the
  deadlock (exit 142); the fixed version passes.
- `meson compile -C build` succeeds and `meson test -C build --print-errorlogs`
  passes 29/29. Socket operations are denied in the sandbox; the passing suite
  and desktop reproductions ran with approved execution outside it.

**Not verified:** visual quality, input usability, resize, real-terminal trial.
P4-T12 remains open. Continue the Project 21 work order in
[STATUS.md](../STATUS.md); the older hypotheses below are historical and do
not explain this reproduced startup deadlock.

## Historical record

Renamed from `05.md` on 2026-09-29 so that handoff 05 stays reserved for
Project 5. Content below is the 2026-09-28 author's record; a615f18 later added
the `wl_seat.capabilities` handler discussed under "Input not working" and fixed
listener crashes. No runtime observation of this host window exists. The same
change reintroduced direct `/dev/input` evdev reading in windowed mode
(`input_init()`); P4-T12 owns the follow-up.

## Context

The original X11 host window on KDE Plasma/KWin Wayland caused flickering due to
a triple-layer presentation mismatch: EGL → X11 → Xwayland → KWin. This handoff
documents the migration attempt to a native Wayland client presentation.

## Changes

### display.h
- Replaced X11 fields (`display`, `xwin`, `colormap`) with Wayland fields:
  `wl_client_display`, `wl_surface`, `wl_egl_window` — all default `= nullptr`.
- Added `#include <wayland-client.h>` and `#include <wayland-egl.h>`.

### display.cpp
- **Generated protocol:** `#include "xdg-shell-client-protocol.h"` and
  `#include "relative-pointer-client-protocol.h"`.
- **host_window_state struct** extended with seat, keyboard, pointer, and
  relative pointer manager resources.
- **registry_global():** binds xdg_wm_base, wl_compositor, wl_seat (v7),
  and zwp_relative_pointer_manager_v1.
- **Event listeners added:**
  - `wl_keyboard_listener`: key (WASD mapping via keycodes 26/38/39/40),
    modifiers (no-op).
  - `wl_pointer_listener`: motion (no-op, position tracked but unused).
  - `zwp_relative_pointer_v1_listener`: relative_motion → calls
    `input_wayland_pointer_motion(dx_unaccel, dy_unaccel)`.
- **setup_wayland_client_window():** creates the full Wayland client window
  pipeline: wl_display connect → registry → xdg_wm_base → wl_surface →
  xdg_surface/xdg_toplevel → wl_egl_window → wl_seat/keyboard/pointer/
  relative_pointer setup → wl_surface_commit → roundtrip.
- **destroy_wayland_client_window():** destroys wl_egl_window, wl_surface,
  and disconnects wl_display. Cleans up g_hws.
- **create_egl_for_wayland():** uses `EGL_PLATFORM_WAYLAND_EXT` with our
  `wl_display` pointer. Creates EGL context and surface from `wl_egl_window`.
  Enables vsync (`eglSwapInterval(egl_dpy, 1)`).
- **swap_buffers():** removed manual `wl_surface_commit()` +
  `wl_display_flush()`. Mesa's Wayland EGL backend commits internally.

### input.h / input.cpp
- Removed X11 includes and X11 globals.
- **input_process():** opens evdev devices, reads events via poll(),
  updates `live_w/a/s/d/live_mouse_x/y` for camera movement, forwards
  key/button events to Wayland clients. No-op in headless mode.
- **input_process_wayland_client():** dispatches pending events from
  `wl_client_display` and flushes. Returns -1 if window was closed.
- **input_handle_window_close():** sets `g_window_closed = true`.
- **input_wayland_key():** called from the keyboard listener; updates
  live WASD state.
- **input_wayland_pointer_motion():** called from the relative pointer
  listener; updates live mouse delta.

### meson.build
- Removed x11 from mansion_exe dependencies.
- Added wayland_client to mansion_exe dependencies.
- Added xdg_shell_client_h, xdg_shell_code, and relative_ptr_client_h,
  relative_ptr_code to mansion_exe sources.

## Verification

- **Build:** `meson compile -C build` succeeds (warnings only).
- **Tests:** all 25/25 `meson test -C build` pass.
- **Not verified (runtime):**
  - Flickering in windowed mode (`--room-camera`) is reported as "terrible"
    (worse than before the migration).
  - No mouse or keyboard input received from the host compositor (KWin).

## Known Issues

### Flickering root cause hypothesis

The flickering is likely caused by Mesa's Wayland EGL backend creating its own
internal `wl_display` connection for buffer management, separate from our
`wl_client_display`. This means:
1. Our `wl_surface` is created on `wl_client_display`.
2. Mesa's `wl_egl_window` uses the same surface.
3. But Mesa's Wayland EGL backend manages buffers through its own connection.
4. `eglSwapBuffers()` commits buffers on Mesa's internal connection.
5. Our `wl_surface_commit()` (when it was present) commits on our connection.
6. KWin receives conflicting commits from two different connections → flickering.

### Input not working

The seat, keyboard, pointer, and relative pointer resources are created and
listeners are installed. But no events arrive from KWin. Possible causes:
1. The pointer must be focused on the window (requires pointer enter event).
2. The relative pointer protocol requires the pointer to be over our window.
3. KWin may not send pointer events to windows without proper focus.
4. The seat.capabilities event may not fire (listener is nullptr).

### Mesa Wayland EGL backend connection behavior

Key question: Does `eglGetPlatformDisplay(EGL_PLATFORM_WAYLAND_EXT, wl_display,
nullptr)` use the provided `wl_display` pointer, or does Mesa create its own
internal connection?

Evidence from `EGL_PLATFORM_WAYLAND_EXT` with `NULL`:
- `eglGetPlatformDisplay(EGL_PLATFORM_WAYLAND_EXT, NULL, NULL)` succeeds and
  returns a valid EGL display.
- This suggests Mesa CAN create its own Wayland display.
- The question is whether passing our `wl_display` pointer forces Mesa to use it,
  or whether Mesa ignores it and creates its own connection anyway.

### Potential fixes to try

1. **Pass NULL to eglGetPlatformDisplay:** Let Mesa create its own connection.
   But then we can't share the `wl_surface` between our connection and Mesa's.

2. **Use EGL_DEFAULT_DISPLAY:** Let Mesa auto-detect. On a Wayland session,
   this should pick Wayland. But the platform might differ.

3. **Use GBM + manual buffer presentation:** Bypass Mesa's Wayland EGL backend.
   Create a GBM device, allocate GEM buffers, present via `wp_linux_drm_syncobj`
   or `wp_linux_buffer_params`. Keeps everything on our connection.

4. **Use EGL_EXT_platform_x11_compatible:** Present through X11/Xwayland with
   explicit compatible mode. May work better than the broken X11 approach.

5. **Wait for KWin to send pointer enter:** The pointer enter event is needed
   before the relative pointer protocol sends motion events. Currently the
   pointer_motion handler is a no-op (doesn't track position). If KWin sends
   pointer enter, the pointer would be focused on our window, and relative
   pointer motion should start flowing.

## Next steps

1. Test the runtime behavior: does flickering persist? Does input work?
2. If flickering persists, try the Mesa Wayland EGL backend fixes above.
3. If input doesn't work, investigate whether pointer enter is received.
   May need to add pointer enter/leave handlers to track focus state.
4. If the seat.capabilities event needs handling (to know when the pointer
   is ready), add a capabilities handler that grabs the pointer.
5. Consider the GBM + manual buffer presentation approach if the Mesa
   Wayland backend issue cannot be resolved.
