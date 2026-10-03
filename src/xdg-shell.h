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
