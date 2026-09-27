#pragma once

#include <wayland-server.h>

struct MansionCompositor;
struct MansionXdgShell;
struct MansionXdgSurface;

struct MansionXdgShell* create_xdg_shell(struct MansionCompositor* compositor,
                                          struct wl_display* display);
void destroy_xdg_shell(struct MansionXdgShell* shell);

/* Called from compositor.cpp when a wl_surface with an xdg_surface role commits.
 * Sends the initial xdg_toplevel.configure / xdg_surface.configure pair once. */
void xdg_shell_on_surface_commit(struct MansionXdgSurface* xdg_surface);

/* Send a new size configure to the toplevel (called on host window resize). */
void xdg_shell_send_configure_resize(struct MansionXdgSurface* xdg_surface,
                                     int32_t width, int32_t height);

/* Check whether an xdg_surface has an active toplevel. */
bool xdg_surface_has_toplevel(struct MansionXdgSurface* xdg_surface);

/* Get the configured width/height from the surface's xdg toplevel (P3-T04). */
void xdg_surface_get_toplevel_size(struct MansionXdgSurface* xdg_surface,
                                   int32_t* out_width, int32_t* out_height);
