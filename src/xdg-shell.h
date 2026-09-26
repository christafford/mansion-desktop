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
