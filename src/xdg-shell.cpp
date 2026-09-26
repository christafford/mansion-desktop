#include <cstring>
#include <iostream>

#include <wayland-server.h>
#include "xdg-shell-server-protocol.h"

#include "xdg-shell.h"
#include "compositor.h"

/* ── Internal structures ─────────────────────────────────────────── */

struct MansionXdgWmBase {
    struct wl_resource* resource;
};

struct MansionXdgSurface {
    struct wl_resource* resource;
    struct wl_resource* surface_resource;  /* associated wl_surface */
    struct wl_resource* toplevel_resource;
    int32_t toplevel_configure_width;
    int32_t toplevel_configure_height;
    bool configured;
    uint32_t configure_serial;
    uint32_t configure_serial_next;       /* monotonically increasing */
    wl_list link;                          /* link in global surface_list */
};

/* Forward declaration — defined below. */
struct MansionXdgSurface;

struct MansionXdgToplevel {
    struct wl_resource* resource;
    struct MansionXdgSurface* parent;   /* parent xdg_surface (for cleanup coordination) */
    int32_t configure_width;
    int32_t configure_height;
};

/* Global list of all xdg_surfaces tracked by this shell instance. */
static wl_list g_surface_list = {0};

/* ── xdg_positioner ──────────────────────────────────────────────── */

static void positioner_destroy(struct wl_client* client, struct wl_resource* resource) {
    (void)client;
    wl_resource_destroy(resource);
}

static const struct xdg_positioner_interface positioner_impl = {
    .destroy = positioner_destroy,
};

/* ── xdg_wm_base ─────────────────────────────────────────────────── */

static void wm_base_destroy(struct wl_client* client, struct wl_resource* resource) {
    (void)client;
    wl_resource_destroy(resource);
}

static void wm_base_create_positioner(struct wl_client* client, struct wl_resource* resource,
                                       uint32_t id) {
    (void)client; (void)resource;
    /* Positioner not yet implemented; create a stub object and reject on use */
    struct wl_resource* pos = wl_resource_create(client, &xdg_positioner_interface,
                                                  1, id);
    if (pos) {
        wl_resource_set_implementation(pos, &positioner_impl, nullptr, nullptr);
    }
}

static void wm_base_pong(struct wl_client* client, struct wl_resource* resource, uint32_t serial) {
    (void)client; (void)resource; (void)serial;
    /* Pong received from client — nothing to do on the server side */
}

/* Forward declaration — wm_base_get_xdg_surface is defined below. */
static void wm_base_get_xdg_surface(struct wl_client* client, struct wl_resource* resource,
                                     uint32_t id, struct wl_resource* surface_resource);

static const struct xdg_wm_base_interface wm_base_impl = {
    .destroy = wm_base_destroy,
    .create_positioner = wm_base_create_positioner,
    .get_xdg_surface = wm_base_get_xdg_surface,
    .pong = wm_base_pong,
};

/* ── xdg_toplevel ────────────────────────────────────────────────── */

static void toplevel_destroy(struct wl_client* client, struct wl_resource* resource) {
    (void)client;
    auto* toplevel = static_cast<MansionXdgToplevel*>(wl_resource_get_user_data(resource));
    if (!toplevel) return;
    /* Clear parent's reference to prevent double-destroy of the toplevel resource */
    if (toplevel->parent) {
        toplevel->parent->toplevel_resource = nullptr;
        toplevel->parent = nullptr;
    }
    wl_resource_destroy(resource);
    wl_resource_set_user_data(resource, nullptr);
    delete toplevel;
}

static void toplevel_set_title(struct wl_client* client, struct wl_resource* resource,
                                const char* title) {
    (void)client; (void)resource; (void)title;
}

static void toplevel_set_app_id(struct wl_client* client, struct wl_resource* resource,
                                 const char* app_id) {
    (void)client; (void)resource; (void)app_id;
}

static void toplevel_show_window_menu(struct wl_client* client, struct wl_resource* resource,
                                       struct wl_resource* seat, uint32_t serial,
                                       int32_t x, int32_t y) {
    (void)client; (void)resource; (void)seat; (void)serial; (void)x; (void)y;
}

static void toplevel_move(struct wl_client* client, struct wl_resource* resource,
                           struct wl_resource* seat, uint32_t serial) {
    (void)client; (void)resource; (void)seat; (void)serial;
}

static void toplevel_resize(struct wl_client* client, struct wl_resource* resource,
                             struct wl_resource* seat, uint32_t serial, uint32_t edges) {
    (void)client; (void)resource; (void)seat; (void)serial; (void)edges;
}

static void toplevel_set_max_size(struct wl_client* client, struct wl_resource* resource,
                                   int32_t width, int32_t height) {
    (void)client; (void)resource; (void)width; (void)height;
}

static void toplevel_set_min_size(struct wl_client* client, struct wl_resource* resource,
                                   int32_t width, int32_t height) {
    (void)client; (void)resource; (void)width; (void)height;
}

static void toplevel_set_maximized(struct wl_client* client, struct wl_resource* resource) {
    (void)client; (void)resource;
}

static void toplevel_unset_maximized(struct wl_client* client, struct wl_resource* resource) {
    (void)client; (void)resource;
}

static void toplevel_set_fullscreen(struct wl_client* client, struct wl_resource* resource,
                                     struct wl_resource* output) {
    (void)client; (void)resource; (void)output;
}

static void toplevel_unset_fullscreen(struct wl_client* client, struct wl_resource* resource) {
    (void)client; (void)resource;
}

static void toplevel_set_minimized(struct wl_client* client, struct wl_resource* resource) {
    (void)client; (void)resource;
}

static void toplevel_set_parent(struct wl_client* client, struct wl_resource* resource,
                                 struct wl_resource* parent) {
    (void)client; (void)resource; (void)parent;
}

static const struct xdg_toplevel_interface toplevel_impl = {
    .destroy = toplevel_destroy,
    .set_parent = toplevel_set_parent,
    .set_title = toplevel_set_title,
    .set_app_id = toplevel_set_app_id,
    .show_window_menu = toplevel_show_window_menu,
    .move = toplevel_move,
    .resize = toplevel_resize,
    .set_max_size = toplevel_set_max_size,
    .set_min_size = toplevel_set_min_size,
    .set_maximized = toplevel_set_maximized,
    .unset_maximized = toplevel_unset_maximized,
    .set_fullscreen = toplevel_set_fullscreen,
    .unset_fullscreen = toplevel_unset_fullscreen,
    .set_minimized = toplevel_set_minimized,
};

/* ── xdg_surface ─────────────────────────────────────────────────── */

static void xdg_surface_destroy(struct wl_client* client, struct wl_resource* resource) {
    (void)client;
    auto* xdg_surface = static_cast<MansionXdgSurface*>(wl_resource_get_user_data(resource));
    if (!xdg_surface) return;
    if (xdg_surface->toplevel_resource) {
        wl_resource_destroy(xdg_surface->toplevel_resource);
        xdg_surface->toplevel_resource = nullptr;
    }
    wl_list_remove(&xdg_surface->link);
    wl_resource_destroy(resource);
    xdg_surface->resource = nullptr;
    delete xdg_surface;
}

static void xdg_surface_get_toplevel(struct wl_client* client, struct wl_resource* resource,
                                      uint32_t id) {
    (void)client;
    auto* xdg_surface = static_cast<MansionXdgSurface*>(wl_resource_get_user_data(resource));

    /* Reject duplicate toplevel request */
    if (xdg_surface->toplevel_resource) {
        wl_resource_post_error(resource, XDG_SURFACE_ERROR_ALREADY_CONSTRUCTED,
                                "xdg_surface has already been given a toplevel");
        return;
    }

    auto* toplevel = new MansionXdgToplevel;
    toplevel->resource = wl_resource_create(client, &xdg_toplevel_interface,
                                             wl_resource_get_version(resource), id);
    if (!toplevel->resource) {
        delete toplevel;
        wl_client_post_no_memory(client);
        return;
    }
    wl_resource_set_implementation(toplevel->resource, &toplevel_impl, toplevel, nullptr);
    toplevel->parent = xdg_surface;
    xdg_surface->toplevel_resource = toplevel->resource;

    /* Store the default configure size */
    toplevel->configure_width = 800;
    toplevel->configure_height = 600;
    xdg_surface->toplevel_configure_width = 800;
    xdg_surface->toplevel_configure_height = 600;
}

static void xdg_surface_ack_configure(struct wl_client* client, struct wl_resource* resource,
                                       uint32_t serial) {
    (void)client;
    auto* xdg_surface = static_cast<MansionXdgSurface*>(wl_resource_get_user_data(resource));
    xdg_surface->configure_serial = serial;
}

static void xdg_surface_set_window_geometry(struct wl_client* client, struct wl_resource* resource,
                                             int32_t x, int32_t y, int32_t width, int32_t height) {
    (void)client; (void)resource; (void)x; (void)y; (void)width; (void)height;
}

static const struct xdg_surface_interface xdg_surface_impl = {
    .destroy = xdg_surface_destroy,
    .get_toplevel = xdg_surface_get_toplevel,
    .get_popup = nullptr,
    .set_window_geometry = xdg_surface_set_window_geometry,
    .ack_configure = xdg_surface_ack_configure,
};

/* ── Surface → xdg_surface mapping ───────────────────────────────── */

/* Iterate all xdg_surfaces and send configure for unconfigured ones
 * whose wl_surface just committed. */
void xdg_shell_on_surface_commit(struct wl_resource* surface_resource) {
    MansionXdgSurface* xdg_surface;
    wl_list_for_each(xdg_surface, &g_surface_list, link) {
        if (xdg_surface->surface_resource == surface_resource && !xdg_surface->configured) {
            /* Send xdg_surface.configure first (sends serial) */
            uint32_t serial = xdg_surface->configure_serial_next++;
            xdg_surface_send_configure(xdg_surface->resource, serial);

            /* Then send xdg_toplevel.configure with the size hint */
            if (xdg_surface->toplevel_resource) {
                struct wl_array empty_states;
                wl_array_init(&empty_states);
                xdg_toplevel_send_configure(xdg_surface->toplevel_resource,
                                            xdg_surface->toplevel_configure_width,
                                            xdg_surface->toplevel_configure_height,
                                            &empty_states);
                wl_array_release(&empty_states);
            }

            xdg_surface->configured = true;
            return;
        }
    }
}

/* ── xdg_wm_base global ──────────────────────────────────────────── */

struct MansionXdgShell {
    struct wl_global* global;
};

static void wm_base_bind(struct wl_client* client, void* data, uint32_t version, uint32_t id) {
    (void)version;
    auto* shell = static_cast<MansionXdgShell*>(data);

    auto* wm_base = new MansionXdgWmBase;
    wm_base->resource = wl_resource_create(client, &xdg_wm_base_interface, version, id);
    if (!wm_base->resource) {
        delete wm_base;
        wl_client_post_no_memory(client);
        return;
    }
    wl_resource_set_implementation(wm_base->resource, &wm_base_impl, wm_base, nullptr);
    (void)shell;
}

static void wm_base_get_xdg_surface(struct wl_client* client, struct wl_resource* resource,
                                     uint32_t id, struct wl_resource* surface_resource) {
    (void)client; (void)resource;
    auto* xdg_surface = new MansionXdgSurface;
    xdg_surface->resource = wl_resource_create(client, &xdg_surface_interface,
                                                wl_resource_get_version(resource), id);
    if (!xdg_surface->resource) {
        delete xdg_surface;
        wl_client_post_no_memory(client);
        return;
    }
    wl_resource_set_implementation(xdg_surface->resource, &xdg_surface_impl, xdg_surface, nullptr);
    xdg_surface->surface_resource = surface_resource;
    xdg_surface->toplevel_resource = nullptr;
    xdg_surface->configured = false;
    xdg_surface->configure_serial = 0;
    xdg_surface->configure_serial_next = 1;
    xdg_surface->toplevel_configure_width = 800;
    xdg_surface->toplevel_configure_height = 600;
    wl_list_init(&xdg_surface->link);
    wl_list_insert(&g_surface_list, &xdg_surface->link);
}

struct MansionXdgShell* create_xdg_shell(struct MansionCompositor* compositor,
                                          struct wl_display* display) {
    (void)compositor;
    auto* shell = new MansionXdgShell;

    /* Initialize the global surface tracking list */
    wl_list_init(&g_surface_list);

    shell->global = wl_global_create(display, &xdg_wm_base_interface, 3,
                                      shell, wm_base_bind);
    if (!shell->global) {
        delete shell;
        return nullptr;
    }

    return shell;
}

void destroy_xdg_shell(struct MansionXdgShell* shell) {
    if (!shell) return;
    wl_global_destroy(shell->global);

    /* Clean up all tracked xdg_surfaces */
    MansionXdgSurface* xdg_surface, *next;
    wl_list_for_each_safe(xdg_surface, next, &g_surface_list, link) {
        if (xdg_surface->toplevel_resource) {
            wl_resource_destroy(xdg_surface->toplevel_resource);
            xdg_surface->toplevel_resource = nullptr;
        }
        if (xdg_surface->resource) {
            wl_resource_destroy(xdg_surface->resource);
            xdg_surface->resource = nullptr;
        }
        wl_list_remove(&xdg_surface->link);
        delete xdg_surface;
    }

    delete shell;
}
