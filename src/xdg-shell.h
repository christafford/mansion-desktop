#pragma once

#include <wayland-server.h>

struct MansionCompositor;
struct MansionXdgShell;

struct MansionXdgShell* create_xdg_shell(struct MansionCompositor* compositor,
                                          struct wl_display* display);
void destroy_xdg_shell(struct MansionXdgShell* shell);

/* Called from compositor.cpp when a wl_surface commits.
 * Sends the initial toplevel configure if the surface has an xdg_toplevel
 * that hasn't been configured yet. */
void xdg_shell_on_surface_commit(struct wl_resource* surface_resource);
