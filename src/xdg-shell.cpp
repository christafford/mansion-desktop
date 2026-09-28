// xdg_wm_base / xdg_surface / xdg_toplevel (P1-T03).
//
// Ownership: every Mansion* struct here is owned by its wl_resource and freed
// from the resource destructor, so client disconnects (which destroy resources
// without calling the `destroy` request handlers) never leak or double-free.
// Cross-links between xdg_surface and xdg_toplevel, and between xdg_surface and
// the wl_surface, are cleared by whichever side goes away first.

#include <cstdio>
#include <wayland-server.h>
#include "xdg-shell-server-protocol.h"

#include "xdg-shell.h"
#include "compositor.h"
#include "compositor-private.h"

namespace {

constexpr uint32_t kWmBaseVersion = 3;
constexpr int32_t kInitialWidth = 800;
constexpr int32_t kInitialHeight = 600;

struct MansionXdgToplevel;

} // namespace

struct MansionXdgShell {
    struct wl_global* global;
};

struct MansionXdgSurface {
    struct wl_resource* resource;
    struct wl_resource* surface_resource;   // nullptr once the wl_surface is gone
    MansionXdgToplevel* toplevel;           // nullptr until get_toplevel / after destroy
    struct wl_listener surface_destroy;
    struct wl_list toplevel_link;           // P5-T01: link in compositor toplevel_list
    bool configured;
    uint32_t last_configure_serial;
    uint32_t acked_serial;
};

namespace {

struct MansionXdgToplevel {
    struct wl_resource* resource;
    MansionXdgSurface* xdg_surface;         // nullptr once the xdg_surface is gone
    int32_t width, height;                  // size sent in the initial configure
    /* P3-T04: saved size for restoring when exiting Application mode. */
    int32_t saved_width, saved_height;
    bool has_saved_size = false;
};

template <typename T>
T* user_data(struct wl_resource* resource) {
    return static_cast<T*>(wl_resource_get_user_data(resource));
}

// Every `destroy` request just drops the resource; the destructor does the work.
void resource_destroy_request(struct wl_client*, struct wl_resource* resource) {
    wl_resource_destroy(resource);
}

/* ── xdg_positioner: accepted and ignored until popups exist ─────── */

void positioner_ignore_2i(struct wl_client*, struct wl_resource*, int32_t, int32_t) {}
void positioner_ignore_4i(struct wl_client*, struct wl_resource*, int32_t, int32_t, int32_t, int32_t) {}
void positioner_ignore_u(struct wl_client*, struct wl_resource*, uint32_t) {}
void positioner_ignore(struct wl_client*, struct wl_resource*) {}

const struct xdg_positioner_interface positioner_impl = {
    .destroy = resource_destroy_request,
    .set_size = positioner_ignore_2i,
    .set_anchor_rect = positioner_ignore_4i,
    .set_anchor = positioner_ignore_u,
    .set_gravity = positioner_ignore_u,
    .set_constraint_adjustment = positioner_ignore_u,
    .set_offset = positioner_ignore_2i,
    .set_reactive = positioner_ignore,
    .set_parent_size = positioner_ignore_2i,
    .set_parent_configure = positioner_ignore_u,
};

/* ── xdg_toplevel ────────────────────────────────────────────────── */

void toplevel_resource_destroyed(struct wl_resource* resource) {
    (void)resource;
    auto* toplevel = user_data<MansionXdgToplevel>(resource);
    if (!toplevel) return;
    MansionXdgSurface* xdg_surf = toplevel->xdg_surface;
    toplevel->xdg_surface = nullptr;
    if (xdg_surf) xdg_surf->toplevel = nullptr;
    /* Do NOT delete toplevel here — defer to xdg_shell destruction. */
}

void toplevel_ignore(struct wl_client*, struct wl_resource*) {}
void toplevel_ignore_s(struct wl_client*, struct wl_resource*, const char*) {}
void toplevel_ignore_r(struct wl_client*, struct wl_resource*, struct wl_resource*) {}
void toplevel_ignore_2i(struct wl_client*, struct wl_resource*, int32_t, int32_t) {}
void toplevel_ignore_ru(struct wl_client*, struct wl_resource*, struct wl_resource*, uint32_t) {}
void toplevel_ignore_ruu(struct wl_client*, struct wl_resource*, struct wl_resource*, uint32_t, uint32_t) {}
void toplevel_ignore_ru2i(struct wl_client*, struct wl_resource*, struct wl_resource*, uint32_t, int32_t, int32_t) {}

const struct xdg_toplevel_interface toplevel_impl = {
    .destroy = resource_destroy_request,
    .set_parent = toplevel_ignore_r,
    .set_title = toplevel_ignore_s,
    .set_app_id = toplevel_ignore_s,
    .show_window_menu = toplevel_ignore_ru2i,
    .move = toplevel_ignore_ru,
    .resize = toplevel_ignore_ruu,
    .set_max_size = toplevel_ignore_2i,
    .set_min_size = toplevel_ignore_2i,
    .set_maximized = toplevel_ignore,
    .unset_maximized = toplevel_ignore,
    .set_fullscreen = toplevel_ignore_r,
    .unset_fullscreen = toplevel_ignore,
    .set_minimized = toplevel_ignore,
};

/* ── xdg_surface ─────────────────────────────────────────────────── */

/* P5-T01: register/unregister a toplevel surface in the compositor's
 * window registry. Called when an xdg_toplevel is created or destroyed.
 * Safe to call even if the surface was already unregistered (e.g. during
 * client disconnect where resource destruction order may call both paths). */
static void toplevel_register(MansionXdgSurface* xdg_surface) {
    if (!xdg_surface || !xdg_surface->surface_resource) return;
    auto* surface = compositor_surface_from_resource(xdg_surface->surface_resource);
    if (!surface || !surface->compositor) return;
    wl_list_init(&xdg_surface->toplevel_link);
    wl_list_insert(surface->compositor->toplevel_list.prev, &xdg_surface->toplevel_link);
    surface->compositor->toplevel_count++;
}

static void toplevel_unregister(MansionXdgSurface* xdg_surface) {
    if (!xdg_surface || !xdg_surface->surface_resource) return;
    struct wl_list* list = xdg_surface->toplevel_link.prev;
    wl_list_remove(&xdg_surface->toplevel_link);
    wl_list_init(&xdg_surface->toplevel_link);
    if (auto* surface = compositor_surface_from_resource(xdg_surface->surface_resource)) {
        if (surface->compositor && list == &surface->compositor->toplevel_list)
            surface->compositor->toplevel_count--;
    }
}

void xdg_surface_detach_wl_surface(MansionXdgSurface* xdg_surface) {
    if (!xdg_surface->surface_resource) return;
    if (auto* surface = compositor_surface_from_resource(xdg_surface->surface_resource)) {
        if (surface->xdg_surface == xdg_surface) surface->xdg_surface = nullptr;
    }
    wl_list_remove(&xdg_surface->surface_destroy.link);
    xdg_surface->surface_resource = nullptr;
}

void xdg_surface_resource_destroyed(struct wl_resource* resource) {
    (void)resource;
    auto* xdg_surface = user_data<MansionXdgSurface>(resource);
    if (!xdg_surface) return;
    /* P5-T01: unregister from the window registry before clearing.
     * If surface_resource is null, the wl_surface was destroyed first
     * and already called toplevel_unregister via on_wl_surface_destroyed. */
    if (xdg_surface->surface_resource)
        toplevel_unregister(xdg_surface);
    if (xdg_surface->toplevel) xdg_surface->toplevel->xdg_surface = nullptr;
    xdg_surface_detach_wl_surface(xdg_surface);
    /* Do NOT delete xdg_surface here — defer to xdg_shell destruction. */
}

// The wl_surface went away first (client disconnect or protocol misuse).
void on_wl_surface_destroyed(struct wl_listener* listener, void*) {
    MansionXdgSurface* xdg_surface = wl_container_of(listener, xdg_surface, surface_destroy);
    /* P5-T01: unregister from the window registry. */
    toplevel_unregister(xdg_surface);
    // The wl_surface's own destructor frees MansionSurface, so only drop our side.
    wl_list_remove(&xdg_surface->surface_destroy.link);
    xdg_surface->surface_resource = nullptr;
}

void xdg_surface_get_toplevel(struct wl_client* client, struct wl_resource* resource, uint32_t id) {
    auto* xdg_surface = user_data<MansionXdgSurface>(resource);
    if (xdg_surface->toplevel) {
        wl_resource_post_error(resource, XDG_SURFACE_ERROR_ALREADY_CONSTRUCTED,
                               "xdg_surface already has a toplevel");
        return;
    }
    auto* toplevel = new MansionXdgToplevel{};
    toplevel->resource = wl_resource_create(client, &xdg_toplevel_interface,
                                            wl_resource_get_version(resource), id);
    if (!toplevel->resource) {
        delete toplevel;
        wl_client_post_no_memory(client);
        return;
    }
    toplevel->xdg_surface = xdg_surface;
    toplevel->width = kInitialWidth;
    toplevel->height = kInitialHeight;
    wl_resource_set_implementation(toplevel->resource, &toplevel_impl, toplevel,
                                   toplevel_resource_destroyed);
    xdg_surface->toplevel = toplevel;
    /* P5-T01: register in the window registry. */
    toplevel_register(xdg_surface);
}

void xdg_surface_get_popup(struct wl_client*, struct wl_resource* resource, uint32_t,
                           struct wl_resource*, struct wl_resource*) {
    wl_resource_post_error(resource, WL_DISPLAY_ERROR_IMPLEMENTATION,
                           "xdg_popup is not implemented yet");
}

void xdg_surface_set_window_geometry(struct wl_client*, struct wl_resource*, int32_t, int32_t,
                                     int32_t, int32_t) {}

void xdg_surface_ack_configure(struct wl_client*, struct wl_resource* resource, uint32_t serial) {
    user_data<MansionXdgSurface>(resource)->acked_serial = serial;
}

const struct xdg_surface_interface xdg_surface_impl = {
    .destroy = resource_destroy_request,
    .get_toplevel = xdg_surface_get_toplevel,
    .get_popup = xdg_surface_get_popup,
    .set_window_geometry = xdg_surface_set_window_geometry,
    .ack_configure = xdg_surface_ack_configure,
};

/* ── xdg_wm_base ─────────────────────────────────────────────────── */

void wm_base_create_positioner(struct wl_client* client, struct wl_resource* resource, uint32_t id) {
    auto* positioner = wl_resource_create(client, &xdg_positioner_interface,
                                          wl_resource_get_version(resource), id);
    if (!positioner) {
        wl_client_post_no_memory(client);
        return;
    }
    wl_resource_set_implementation(positioner, &positioner_impl, nullptr, nullptr);
}

void wm_base_get_xdg_surface(struct wl_client* client, struct wl_resource* resource, uint32_t id,
                             struct wl_resource* surface_resource) {
    auto* surface = compositor_surface_from_resource(surface_resource);
    if (!surface) {
        wl_resource_post_error(resource, XDG_WM_BASE_ERROR_INVALID_SURFACE_STATE,
                               "get_xdg_surface on an unknown wl_surface");
        return;
    }
    if (surface->xdg_surface) {
        wl_resource_post_error(resource, XDG_WM_BASE_ERROR_ROLE,
                               "wl_surface already has an xdg_surface");
        return;
    }
    auto* xdg_surface = new MansionXdgSurface{};
    xdg_surface->resource = wl_resource_create(client, &xdg_surface_interface,
                                               wl_resource_get_version(resource), id);
    if (!xdg_surface->resource) {
        delete xdg_surface;
        wl_client_post_no_memory(client);
        return;
    }
    xdg_surface->surface_resource = surface_resource;
    xdg_surface->surface_destroy.notify = on_wl_surface_destroyed;
    wl_resource_add_destroy_listener(surface_resource, &xdg_surface->surface_destroy);
    surface->xdg_surface = xdg_surface;
    wl_resource_set_implementation(xdg_surface->resource, &xdg_surface_impl, xdg_surface,
                                   xdg_surface_resource_destroyed);
}

void wm_base_pong(struct wl_client*, struct wl_resource*, uint32_t) {}

const struct xdg_wm_base_interface wm_base_impl = {
    .destroy = resource_destroy_request,
    .create_positioner = wm_base_create_positioner,
    .get_xdg_surface = wm_base_get_xdg_surface,
    .pong = wm_base_pong,
};

void wm_base_bind(struct wl_client* client, void* data, uint32_t version, uint32_t id) {
    auto* resource = wl_resource_create(client, &xdg_wm_base_interface, version, id);
    if (!resource) {
        wl_client_post_no_memory(client);
        return;
    }
    wl_resource_set_implementation(resource, &wm_base_impl, data, nullptr);
}

} // namespace

/* ── Public API ──────────────────────────────────────────────────── */

void xdg_shell_on_surface_commit(struct MansionXdgSurface* xdg_surface) {

    // Protocol order: role-specific configure first, then xdg_surface.configure
    // carrying the serial that closes the configure sequence.
    struct wl_array states;
    wl_array_init(&states);
    xdg_toplevel_send_configure(xdg_surface->toplevel->resource,
                                xdg_surface->toplevel->width, xdg_surface->toplevel->height, &states);
    wl_array_release(&states);

    auto* display = wl_client_get_display(wl_resource_get_client(xdg_surface->resource));
    xdg_surface->last_configure_serial = wl_display_next_serial(display);
    xdg_surface_send_configure(xdg_surface->resource, xdg_surface->last_configure_serial);
    xdg_surface->configured = true;
}

void xdg_shell_send_configure_resize(struct MansionXdgSurface* xdg_surface,
                                     int32_t width, int32_t height) {
    if (!xdg_surface || !xdg_surface->toplevel) return;
    xdg_surface->toplevel->width = width;
    xdg_surface->toplevel->height = height;

    struct wl_array states;
    wl_array_init(&states);
    xdg_toplevel_send_configure(xdg_surface->toplevel->resource, width, height, &states);
    wl_array_release(&states);

    auto* display = wl_client_get_display(wl_resource_get_client(xdg_surface->resource));
    uint32_t serial = wl_display_next_serial(display);
    xdg_surface_send_configure(xdg_surface->resource, serial);
}

bool xdg_surface_has_toplevel(struct MansionXdgSurface* xdg_surface) {
    return xdg_surface && xdg_surface->toplevel != nullptr;
}

void xdg_surface_get_toplevel_size(struct MansionXdgSurface* xdg_surface,
                                   int32_t* out_width, int32_t* out_height) {
    if (!xdg_surface || !xdg_surface->toplevel) {
        if (out_width) *out_width = 0;
        if (out_height) *out_height = 0;
        return;
    }
    if (out_width) *out_width = xdg_surface->toplevel->width;
    if (out_height) *out_height = xdg_surface->toplevel->height;
}

/* P3-T04: Save the current toplevel size so it can be restored later. */
void xdg_surface_save_toplevel_size(struct MansionXdgSurface* xdg_surface) {
    if (!xdg_surface || !xdg_surface->toplevel) return;
    xdg_surface->toplevel->saved_width  = xdg_surface->toplevel->width;
    xdg_surface->toplevel->saved_height = xdg_surface->toplevel->height;
    xdg_surface->toplevel->has_saved_size = true;
}

/* P3-T04: Restore the previously-saved toplevel size and re-send the
 * configure so the client knows about the new size. */
void xdg_surface_restore_toplevel_size(struct MansionXdgSurface* xdg_surface) {
    if (!xdg_surface || !xdg_surface->toplevel) return;
    if (!xdg_surface->toplevel->has_saved_size) return;
    xdg_surface->toplevel->width  = xdg_surface->toplevel->saved_width;
    xdg_surface->toplevel->height = xdg_surface->toplevel->saved_height;
    xdg_surface->toplevel->has_saved_size = false;

    /* Re-send the configure so the client is notified. */
    struct wl_array states;
    wl_array_init(&states);
    xdg_toplevel_send_configure(xdg_surface->toplevel->resource,
                                xdg_surface->toplevel->width,
                                xdg_surface->toplevel->height, &states);
    wl_array_release(&states);

    auto* display = wl_client_get_display(wl_resource_get_client(xdg_surface->resource));
    uint32_t serial = wl_display_next_serial(display);
    xdg_surface_send_configure(xdg_surface->resource, serial);
}

struct MansionXdgShell* create_xdg_shell(struct MansionCompositor*, struct wl_display* display) {
    auto* shell = new MansionXdgShell{};
    shell->global = wl_global_create(display, &xdg_wm_base_interface, kWmBaseVersion,
                                     shell, wm_base_bind);
    if (!shell->global) {
        delete shell;
        return nullptr;
    }
    return shell;
}

void destroy_xdg_shell(struct MansionXdgShell* shell) {
    if (!shell) return;
    // Client resources are torn down by wl_display_destroy (their destructors
    // free the Mansion* structs); only the global belongs to us.
    wl_global_destroy(shell->global);
    delete shell;
}
