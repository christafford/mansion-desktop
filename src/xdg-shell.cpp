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
#include "compositor-private.h"
#include "seat.h"
#include "core-frame-state.h"

#include <cstring>
#include <string>
#include <deque>
#include <iterator>
#include <algorithm>

namespace {

constexpr uint32_t kWmBaseVersion = 3;
constexpr int32_t kInitialWidth = 800;
constexpr int32_t kInitialHeight = 600;

struct MansionXdgToplevel;

} // namespace

struct MansionXdgShell {
    struct wl_global* global;
    struct wl_list resources;
    struct wl_list toplevel_list;   /* list of MansionXdgToplevel* via toplevel_link */
    struct wl_list xdg_surface_list; /* list of MansionXdgSurface* via xdg_surface_link */
};

struct MansionXdgSurface {
    struct wl_resource* resource;
    struct wl_resource* surface_resource;   // nullptr once the wl_surface is gone
    MansionXdgToplevel* toplevel;           // nullptr until get_toplevel / after destroy
    struct wl_listener surface_destroy;
    struct MansionXdgShell* shell;          // back-pointer for registration
    struct wl_list toplevel_link;           // link in compositor toplevel_list
    struct wl_list xdg_surface_link;        // link in shell xdg_surface_list
    bool configured;
    bool has_acked_configure;
    std::deque<uint32_t> pending_configures;
    /* P4-T11: window geometry. */
    int32_t geo_x, geo_y, geo_width, geo_height;
    bool has_geometry = false;
    bool geometry_pending = false;
    XdgWindowState state;
    int32_t pending_min_width = 0, pending_min_height = 0;
    int32_t pending_max_width = 0, pending_max_height = 0;
};

namespace {

struct MansionXdgToplevel {
    struct wl_resource* resource;
    MansionXdgSurface* xdg_surface;         // nullptr once the xdg_surface is gone
    struct wl_list toplevel_link;           // link in MansionXdgShell toplevel_list
    int32_t width, height;                  // size sent in the initial configure
    /* P3-T04: saved size for restoring when exiting Application mode. */
    int32_t saved_width, saved_height;
    bool has_saved_size = false;
    /* P4-T11: toplevel metadata. */
    std::string title;
    std::string app_id;
};

template <typename T>
T* user_data(struct wl_resource* resource) {
    return static_cast<T*>(wl_resource_get_user_data(resource));
}

// Get the MansionXdgShell from a wm_base resource's user data.
MansionXdgShell* shell_from_wm_base(struct wl_resource* resource) {
    return static_cast<MansionXdgShell*>(wl_resource_get_user_data(resource));
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

static void toplevel_unregister(MansionXdgSurface* xdg_surface);

void toplevel_resource_destroyed(struct wl_resource* resource) {
    (void)resource;
    auto* toplevel = user_data<MansionXdgToplevel>(resource);
    if (!toplevel) return;
    MansionXdgSurface* xdg_surf = toplevel->xdg_surface;
    toplevel->xdg_surface = nullptr;
    if (xdg_surf) {
        toplevel_unregister(xdg_surf);
        xdg_surf->toplevel = nullptr;
        xdg_surf->configured = false;
        xdg_surf->has_acked_configure = false;
        xdg_surf->pending_configures.clear();
    }
    wl_list_remove(&toplevel->toplevel_link);
    delete toplevel;
}

void toplevel_ignore(struct wl_client*, struct wl_resource*) {}
void toplevel_ignore_r(struct wl_client*, struct wl_resource*, struct wl_resource*) {}
void toplevel_size_limit(wl_resource* resource, int32_t width, int32_t height, bool minimum) {
    if (width < 0 || height < 0) {
        wl_resource_post_error(resource, XDG_TOPLEVEL_ERROR_INVALID_SIZE, "size limits must be nonnegative");
        return;
    }
    auto* top = user_data<MansionXdgToplevel>(resource);
    if (!top->xdg_surface) return;
    auto* s = top->xdg_surface;
    if (minimum) { s->pending_min_width = width; s->pending_min_height = height; }
    else { s->pending_max_width = width; s->pending_max_height = height; }
}
void toplevel_ignore_ru(struct wl_client*, struct wl_resource*, struct wl_resource*, uint32_t) {}
void toplevel_ignore_ruu(struct wl_client*, struct wl_resource*, struct wl_resource*, uint32_t, uint32_t) {}
void toplevel_ignore_ru2i(struct wl_client*, struct wl_resource*, struct wl_resource*, uint32_t, int32_t, int32_t) {}

const struct xdg_toplevel_interface toplevel_impl = {
    .destroy = resource_destroy_request,
    .set_parent = toplevel_ignore_r,
    .set_title = [](struct wl_client* client, struct wl_resource* resource,
                    const char* title) {
        auto* toplevel = user_data<MansionXdgToplevel>(resource);
        if (!toplevel) return;
        toplevel->title = title ? title : "";
        (void)client;
    },
    .set_app_id = [](struct wl_client* client, struct wl_resource* resource,
                     const char* app_id) {
        auto* toplevel = user_data<MansionXdgToplevel>(resource);
        if (!toplevel) return;
        toplevel->app_id = app_id ? app_id : "";
        (void)client;
    },
    .show_window_menu = toplevel_ignore_ru2i,
    .move = toplevel_ignore_ru,
    .resize = toplevel_ignore_ruu,
    .set_max_size = [](wl_client*, wl_resource* r, int32_t w, int32_t h) { toplevel_size_limit(r, w, h, false); },
    .set_min_size = [](wl_client*, wl_resource* r, int32_t w, int32_t h) { toplevel_size_limit(r, w, h, true); },
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
    auto* surface = user_data<MansionSurface>(xdg_surface->surface_resource);
    if (!surface || !surface->compositor) return;
    wl_list_init(&xdg_surface->toplevel_link);
    wl_list_insert(surface->compositor->toplevel_list.prev, &xdg_surface->toplevel_link);
    surface->compositor->toplevel_count++;
}

static void toplevel_unregister(MansionXdgSurface* xdg_surface) {
    if (!xdg_surface || !xdg_surface->surface_resource) return;
    if (wl_list_empty(&xdg_surface->toplevel_link)) return;
    wl_list_remove(&xdg_surface->toplevel_link);
    wl_list_init(&xdg_surface->toplevel_link);
    if (auto* surface = user_data<MansionSurface>(xdg_surface->surface_resource)) {
        if (surface->compositor) surface->compositor->toplevel_count--;
    }
}

void xdg_surface_detach_wl_surface(MansionXdgSurface* xdg_surface) {
    if (!xdg_surface->surface_resource) return;
    if (auto* surface = user_data<MansionSurface>(xdg_surface->surface_resource)) {
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
    wl_list_remove(&xdg_surface->xdg_surface_link);
    delete xdg_surface;
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
    // Register in the shell's tracking list for cleanup.
    wl_list_init(&toplevel->toplevel_link);
    if (xdg_surface->shell)
        wl_list_insert(&xdg_surface->shell->toplevel_list, &toplevel->toplevel_link);
    /* P5-T01: register in the window registry. */
    toplevel_register(xdg_surface);
}

void xdg_surface_get_popup(struct wl_client*, struct wl_resource* resource, uint32_t,
                           struct wl_resource*, struct wl_resource*) {
    wl_resource_post_error(resource, WL_DISPLAY_ERROR_IMPLEMENTATION,
                           "xdg_popup is not implemented yet");
}

void xdg_surface_set_window_geometry(struct wl_client* client, struct wl_resource* resource,
                                     int32_t x, int32_t y, int32_t w, int32_t h) {
    auto* xdg_surface = user_data<MansionXdgSurface>(resource);
    if (!xdg_surface) return;
    if (w <= 0 || h <= 0) {
        wl_resource_post_error(resource, XDG_SURFACE_ERROR_INVALID_SIZE, "window geometry must be positive");
        return;
    }
    xdg_surface->geo_x = x;
    xdg_surface->geo_y = y;
    xdg_surface->geo_width = w;
    xdg_surface->geo_height = h;
    xdg_surface->geometry_pending = true;
    (void)client;
}

void xdg_surface_ack_configure(struct wl_client* client, struct wl_resource* resource, uint32_t serial) {
    auto* xdg_surface = user_data<MansionXdgSurface>(resource);
    if (!xdg_surface) return;
    auto found = std::find(xdg_surface->pending_configures.begin(),
                           xdg_surface->pending_configures.end(), serial);
    if (found == xdg_surface->pending_configures.end()) {
        wl_resource_post_error(resource, XDG_SURFACE_ERROR_INVALID_SERIAL,
                               "ack_configure serial %u was not pending", serial);
        return;
    }
    xdg_surface->pending_configures.erase(xdg_surface->pending_configures.begin(), std::next(found));
    xdg_surface->has_acked_configure = true;
    xdg_surface->state.acked_serial = serial;
    (void)client;
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
    auto* surface = user_data<MansionSurface>(surface_resource);
    if (!surface) {
        wl_resource_post_error(resource, XDG_WM_BASE_ERROR_INVALID_SURFACE_STATE,
                               "get_xdg_surface on an unknown wl_surface");
        return;
    }
    if (surface->xdg_surface || (surface->core_frame && surface->core_frame->subsurface_role)) {
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
    wl_list_init(&xdg_surface->toplevel_link);
    xdg_surface->surface_resource = surface_resource;
    xdg_surface->surface_destroy.notify = on_wl_surface_destroyed;
    wl_resource_add_destroy_listener(surface_resource, &xdg_surface->surface_destroy);
    xdg_surface->shell = shell_from_wm_base(resource);
    surface->xdg_surface = xdg_surface;
    wl_resource_set_implementation(xdg_surface->resource, &xdg_surface_impl, xdg_surface,
                                   xdg_surface_resource_destroyed);
    // Register in the shell's tracking list for cleanup.
    wl_list_init(&xdg_surface->xdg_surface_link);
    if (xdg_surface->shell)
        wl_list_insert(&xdg_surface->shell->xdg_surface_list, &xdg_surface->xdg_surface_link);
}

void wm_base_pong(struct wl_client*, struct wl_resource*, uint32_t) {}

const struct xdg_wm_base_interface wm_base_impl = {
    .destroy = resource_destroy_request,
    .create_positioner = wm_base_create_positioner,
    .get_xdg_surface = wm_base_get_xdg_surface,
    .pong = wm_base_pong,
};

void wm_base_resource_destroyed(wl_resource* resource) {
    wl_list_remove(wl_resource_get_link(resource));
}
void wm_base_bind(struct wl_client* client, void* data, uint32_t version, uint32_t id) {
    auto* resource = wl_resource_create(client, &xdg_wm_base_interface, version, id);
    if (!resource) {
        wl_client_post_no_memory(client);
        return;
    }
    wl_resource_set_implementation(resource, &wm_base_impl, data, wm_base_resource_destroyed);
    wl_list_insert(&static_cast<MansionXdgShell*>(data)->resources, wl_resource_get_link(resource));
}

} // namespace

/* ── Public API ──────────────────────────────────────────────────── */

static void send_configure(MansionXdgSurface* xdg_surface) {
    // Role configure precedes the serial closing the configure sequence.
    wl_array states;
    wl_array_init(&states);
    xdg_toplevel_send_configure(xdg_surface->toplevel->resource,
                                xdg_surface->toplevel->width, xdg_surface->toplevel->height, &states);
    wl_array_release(&states);
    auto* display = wl_client_get_display(wl_resource_get_client(xdg_surface->resource));
    const uint32_t serial = wl_display_next_serial(display);
    xdg_surface->pending_configures.push_back(serial);
    xdg_surface->state.sent_serial = serial;
    xdg_surface->state.requested_width = xdg_surface->toplevel->width;
    xdg_surface->state.requested_height = xdg_surface->toplevel->height;
    xdg_surface_send_configure(xdg_surface->resource, serial);
    xdg_surface->configured = true;
}

bool xdg_shell_on_surface_commit(MansionXdgSurface* xdg_surface) {
    if (!xdg_surface) return true;
    if (!xdg_surface->toplevel) {
        wl_resource_post_error(xdg_surface->resource, XDG_SURFACE_ERROR_NOT_CONSTRUCTED,
                               "commit requires an xdg_surface role");
        return false;
    }
    auto* surface = user_data<MansionSurface>(xdg_surface->surface_resource);
    if (!xdg_surface->has_acked_configure &&
        (surface->buffer_resource || surface->pending_buffer_resource)) {
        wl_resource_post_error(xdg_surface->resource, XDG_SURFACE_ERROR_UNCONFIGURED_BUFFER,
                               "buffer committed before configure acknowledgement");
        return false;
    }
    if ((xdg_surface->pending_max_width && xdg_surface->pending_min_width > xdg_surface->pending_max_width) ||
        (xdg_surface->pending_max_height && xdg_surface->pending_min_height > xdg_surface->pending_max_height)) {
        wl_resource_post_error(xdg_surface->toplevel->resource, XDG_TOPLEVEL_ERROR_INVALID_SIZE,
                               "minimum size exceeds maximum size");
        return false;
    }
    if (!xdg_surface->configured) send_configure(xdg_surface);
    return true;
}

void xdg_shell_on_surface_unmap(MansionXdgSurface* xdg_surface) {
    xdg_surface->configured = false;
    xdg_surface->has_acked_configure = false;
    xdg_surface->pending_configures.clear();
    xdg_surface->has_geometry = xdg_surface->geometry_pending = false;
    xdg_surface->state = {};
    xdg_surface->pending_min_width = xdg_surface->pending_min_height = 0;
    xdg_surface->pending_max_width = xdg_surface->pending_max_height = 0;
}

void xdg_shell_send_configure_resize(struct MansionXdgSurface* xdg_surface,
                                     int32_t width, int32_t height) {
    if (!xdg_surface || !xdg_surface->toplevel) return;
    xdg_surface->toplevel->width = width;
    xdg_surface->toplevel->height = height;

    send_configure(xdg_surface);
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

    send_configure(xdg_surface);
}

/* P5-T02: cycle keyboard focus through the toplevel list. */
void xdg_shell_cycle_focus(struct MansionSeat* seat,
                            struct MansionCompositor* comp,
                            bool forward) {
    if (!seat || !comp || !comp->seat) return;
    if (wl_list_empty(&comp->toplevel_list) || comp->toplevel_count < 2)
        return;

    struct wl_resource* current = comp->focused_surface_resource;
    MansionXdgSurface* pos;

    if (forward) {
        /* Forward: find current, move to next. Wrap to first if current is last. */
        struct wl_resource* next_surface = nullptr;
        bool found_current = !current;

        wl_list_for_each(pos, &comp->toplevel_list, toplevel_link) {
            if (!pos->toplevel) continue;
            if (pos->surface_resource == nullptr) continue;
            if (found_current) {
                next_surface = pos->surface_resource;
                break;
            }
            if (pos->surface_resource == current) {
                found_current = true;
            }
        }
        /* Wrap: if current was last (or not found), go to the first entry. */
        if (!next_surface) {
            pos = wl_container_of(comp->toplevel_list.next, pos, toplevel_link);
            if (pos->surface_resource) {
                next_surface = pos->surface_resource;
            }
        }
        if (next_surface) {
            seat_set_keyboard_focus(seat, next_surface, comp);
        }
    } else {
        /* Backward: find current, move to previous. Wrap to last if current is first. */
        struct wl_resource* prev_surface = nullptr;
        struct wl_resource* first_surface = nullptr;
        bool found_current = false;
        MansionXdgSurface* last_entry = nullptr;

        wl_list_for_each(pos, &comp->toplevel_list, toplevel_link) {
            if (!pos->toplevel) continue;
            if (pos->surface_resource == nullptr) continue;

            if (!first_surface) {
                first_surface = pos->surface_resource;
            }

            if (pos->surface_resource == current) {
                found_current = true;
                break;
            }
            prev_surface = pos->surface_resource;
            last_entry = pos;
        }

        struct wl_resource* target = nullptr;
        if (found_current) {
            if (prev_surface) {
                target = prev_surface;
            } else if (last_entry && last_entry->surface_resource) {
                /* current is first — wrap to last */
                target = last_entry->surface_resource;
            }
        }
        if (target && target != current) {
            seat_set_keyboard_focus(seat, target, comp);
        }
    }
}

struct MansionXdgShell* create_xdg_shell(struct MansionCompositor*, struct wl_display* display) {
    auto* shell = new MansionXdgShell{};
    wl_list_init(&shell->resources);
    wl_list_init(&shell->toplevel_list);
    wl_list_init(&shell->xdg_surface_list);
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
    // Free all xdg_toplevel objects.
    MansionXdgToplevel *toplevel, *toplevel_next;
    wl_list_for_each_safe(toplevel, toplevel_next, &shell->toplevel_list, toplevel_link) {
        wl_resource_destroy(toplevel->resource);
    }
    // Free all xdg_surface objects.
    MansionXdgSurface *xdg_surf, *xdg_surf_next;
    wl_list_for_each_safe(xdg_surf, xdg_surf_next, &shell->xdg_surface_list, xdg_surface_link) {
        wl_resource_destroy(xdg_surf->resource);
    }
    while (!wl_list_empty(&shell->resources))
        wl_resource_destroy(wl_resource_from_link(shell->resources.next));
    wl_global_destroy(shell->global);
    delete shell;
}

void xdg_shell_on_surface_applied(MansionXdgSurface* s, int32_t width, int32_t height, int32_t x, int32_t y) {
    if (!s || !s->toplevel) return;
    s->state.min_width = s->pending_min_width; s->state.min_height = s->pending_min_height;
    s->state.max_width = s->pending_max_width; s->state.max_height = s->pending_max_height;
    s->state.committed_serial = s->state.acked_serial;
    if (width <= 0 || height <= 0) return;
    if (s->geometry_pending) {
        // Effective geometry is clamped when applied, and remains fixed until
        // set_window_geometry is applied again (xdg-shell specification).
        const int64_t left = std::max<int64_t>(x, s->geo_x);
        const int64_t top = std::max<int64_t>(y, s->geo_y);
        const int64_t right = std::min<int64_t>(int64_t(x) + width, int64_t(s->geo_x) + s->geo_width);
        const int64_t bottom = std::min<int64_t>(int64_t(y) + height, int64_t(s->geo_y) + s->geo_height);
        if (right <= left || bottom <= top) {
            wl_resource_post_error(s->resource, XDG_SURFACE_ERROR_INVALID_SIZE, "window geometry does not intersect surface");
            return;
        }
        s->state.x = left; s->state.y = top;
        s->state.width = right - left; s->state.height = bottom - top;
        s->state.declared_width = s->geo_width; s->state.declared_height = s->geo_height;
        s->state.geometry_is_set = true;
        s->has_geometry = true;
        s->geometry_pending = false;
    } else if (!s->has_geometry) {
        s->state.x = x; s->state.y = y;
        s->state.width = width; s->state.height = height;
        s->state.declared_width = width; s->state.declared_height = height;
    }
}

XdgWindowState xdg_surface_window_state(MansionXdgSurface* s) {
    return s && s->toplevel ? s->state : XdgWindowState{};
}

bool xdg_shell_request_resize(MansionXdgSurface* s, int32_t width, int32_t height) {
    if (!s || !s->toplevel || !s->has_acked_configure || width < 1 || height < 1 || width > 2048 || height > 2048)
        return false;
    width = std::max(width, s->state.min_width);
    height = std::max(height, s->state.min_height);
    if (s->state.max_width) width = std::min(width, s->state.max_width);
    if (s->state.max_height) height = std::min(height, s->state.max_height);
    if (width > 2048 || height > 2048) return false;
    // Do not repeatedly configure a client that rounds or declines our size.
    if (width == s->state.requested_width && height == s->state.requested_height) return true;
    if (s->pending_configures.size() >= 64) return false;
    xdg_shell_send_configure_resize(s, width, height);
    return true;
}
