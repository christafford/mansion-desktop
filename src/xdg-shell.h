#pragma once

#include <wayland-server.h>

struct MansionCompositor;
struct MansionXdgShell;
struct MansionXdgSurface;

struct MansionXdgShell* create_xdg_shell(struct MansionCompositor* compositor,
                                          struct wl_display* display);
void destroy_xdg_shell(struct MansionXdgShell* shell);

/* Called by either frontend when an xdg surface commits. Sends the initial
 * configure pair once; false means a protocol error (do not consume pixels). */
bool xdg_shell_on_surface_commit(struct MansionXdgSurface* xdg_surface);

/* Reset the initial handshake after an explicit content detach. */
void xdg_shell_on_surface_unmap(struct MansionXdgSurface* xdg_surface);

/* Send a new size configure to the toplevel (called on host window resize). */
void xdg_shell_send_configure_resize(struct MansionXdgSurface* xdg_surface,
                                     int32_t width, int32_t height);

/* Check whether an xdg_surface has an active toplevel. */
bool xdg_surface_has_toplevel(struct MansionXdgSurface* xdg_surface);

/* Get the configured width/height from the surface's xdg toplevel (P3-T04). */
void xdg_surface_get_toplevel_size(struct MansionXdgSurface* xdg_surface,
                                   int32_t* out_width, int32_t* out_height);

/* Save the current size and restore a previously-saved size (P3-T04).
 * Called when entering/exiting Application mode so the surface can
 * be resized to fullscreen and back. */
void xdg_surface_save_toplevel_size(struct MansionXdgSurface* xdg_surface);
void xdg_surface_restore_toplevel_size(struct MansionXdgSurface* xdg_surface);

/* P5-T02: cycle focus through the toplevel list. */
void xdg_shell_cycle_focus(struct MansionSeat* seat,
                            struct MansionCompositor* comp,
                            bool forward);

// Value metadata: geometry/limits and committed_serial apply at surface commit;
// request/ack fields track protocol progress separately. Geometry is in logical
// surface coordinates; requested size is in window-geometry units. The runtime
// translates x/y into snapshot-canvas coordinates and supplies root dimensions.
struct XdgWindowState {
    int32_t x = 0, y = 0, width = 0, height = 0;
    int32_t surface_width = 0, surface_height = 0;
    int32_t declared_width = 0, declared_height = 0;
    bool geometry_is_set = false;
    int32_t min_width = 0, min_height = 0, max_width = 0, max_height = 0;
    int32_t requested_width = 0, requested_height = 0;
    uint32_t sent_serial = 0, acked_serial = 0, committed_serial = 0;
};
void xdg_shell_on_surface_applied(MansionXdgSurface* surface, int32_t width, int32_t height, int32_t x = 0, int32_t y = 0);
XdgWindowState xdg_surface_window_state(MansionXdgSurface* surface);
// Returns false for invalid state/sizes or a full (64-entry) configure queue.
bool xdg_shell_request_resize(MansionXdgSurface* surface, int32_t width, int32_t height);
