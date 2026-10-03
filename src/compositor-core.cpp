#include <cstring>
#include <algorithm>
#include <new>

#include <wayland-server-protocol.h>
#include <wayland-util.h>

#include "compositor-core.h"
#include "compositor-private.h"
#include "xdg-shell.h"

/* ---------- surface ---------- */

static void frame_callback_destroyed(struct wl_resource* resource) {
    wl_list_remove(wl_resource_get_link(resource));
}

static void surface_destroy_callback(struct wl_resource* resource) {
    auto* surface = static_cast<MansionSurface*>(wl_resource_get_user_data(resource));
    if (!surface) return;
    wl_list_remove(&surface->link);
    wl_list_remove(&surface->buffer_destroy_listener.link);
    wl_list_init(&surface->buffer_destroy_listener.link);
    while (!wl_list_empty(&surface->frame_callback_list)) {
        wl_resource_destroy(wl_resource_from_link(surface->frame_callback_list.next));
    }
    if (surface->compositor->focused_surface_resource == resource) {
        compositor_core_clear_focus(surface->compositor);
    }
    // A renderer-independent core does not retain dead Wayland resources as art.
    // A future owned frame snapshot has its own lifetime, independent of this.
    wl_resource_set_user_data(resource, nullptr);
    delete surface;
}

static void surface_destroy(struct wl_client* client, struct wl_resource* resource) {
    (void)client;
    wl_resource_destroy(resource);
}

/* Buffer destroy listener — fires when the client destroys the wl_buffer proxy. */
static void buffer_destroy_notify(struct wl_listener* listener, void* data) {
    MansionSurface* surface = wl_container_of(listener, surface, buffer_destroy_listener);
    wl_list_remove(&listener->link);
    wl_list_init(&listener->link);
    if (surface->buffer_resource == data) surface->buffer_resource = nullptr;
    if (surface->pending_buffer_resource == data) surface->pending_buffer_resource = nullptr;
    surface->buffer_destroyed = true;
}

static void surface_attach(struct wl_client* client, struct wl_resource* resource,
                           struct wl_resource* buffer_resource, int32_t x, int32_t y) {
    (void)client; (void)x; (void)y;
    auto* surface = static_cast<MansionSurface*>(wl_resource_get_user_data(resource));

    /* Always remove the listener — it may be registered for the current
     * buffer from a previous commit. We will re-register it for the new
     * pending buffer below (or not at all if the new buffer is null). */
    if (!wl_list_empty(&surface->buffer_destroy_listener.link)) {
        wl_list_remove(&surface->buffer_destroy_listener.link);
        wl_list_init(&surface->buffer_destroy_listener.link);
    }

    surface->pending_buffer_resource = buffer_resource;
    surface->has_pending_position = true;
    surface->pending_x = x;
    surface->pending_y = y;

    if (buffer_resource != nullptr) {
        /* Validate stride for shm buffers. */
        auto* shm_buf = wl_shm_buffer_get(buffer_resource);
        if (shm_buf) {
            int32_t stride = wl_shm_buffer_get_stride(shm_buf);
            int32_t fmt_w = wl_shm_buffer_get_width(shm_buf);
            /* Stride must be at least enough for one pixel row of the given format. */
            int32_t min_stride = fmt_w * 4;  /* ARGB8888 = 4 bytes/pixel */
            if (stride < min_stride || stride % 4 != 0) {
                wl_resource_post_error(buffer_resource, WL_SHM_ERROR_INVALID_STRIDE,
                    "invalid stride %d for width %d (min %d)",
                    stride, fmt_w, min_stride);
                surface->pending_buffer_resource = nullptr;
                return;
            }
        }

        surface->buffer_destroyed = false;
        /* Listen for pending buffer destruction. */
        surface->buffer_destroy_listener.notify = buffer_destroy_notify;
        wl_resource_add_destroy_listener(buffer_resource, &surface->buffer_destroy_listener);
    } else {
        surface->buffer_destroyed = false;
    }
}

static void surface_damage(struct wl_client* client, struct wl_resource* resource,
                           int32_t x, int32_t y, int32_t width, int32_t height) {
    (void)client;
    auto* surface = static_cast<MansionSurface*>(wl_resource_get_user_data(resource));
    (void)surface; (void)x; (void)y; (void)width; (void)height;
    /* Accumulate damage into a single bounding box. */
    if (surface->damage_count == 0) {
        surface->pending_x_damage = x;
        surface->pending_y_damage = y;
        surface->pending_w_damage = width;
        surface->pending_h_damage = height;
    } else {
        /* Expand bounding box to include new damage. */
        if (x < surface->pending_x_damage)
            surface->pending_x_damage = x;
        if (y < surface->pending_y_damage)
            surface->pending_y_damage = y;
        int32_t right = x + width;
        int32_t bottom = y + height;
        int32_t cur_right = surface->pending_x_damage + surface->pending_w_damage;
        int32_t cur_bottom = surface->pending_y_damage + surface->pending_h_damage;
        if (right > cur_right)
            surface->pending_w_damage = right - surface->pending_x_damage;
        if (bottom > cur_bottom)
            surface->pending_h_damage = bottom - surface->pending_y_damage;
    }
    surface->damage_count++;
}

static void surface_commit(struct wl_client* client, struct wl_resource* resource) {
    (void)client;
    auto* surface = static_cast<MansionSurface*>(wl_resource_get_user_data(resource));

    if (surface->xdg_surface) xdg_shell_on_surface_commit(surface->xdg_surface);

    /* Apply pending position. */
    if (surface->has_pending_position) {
        surface->current_x = surface->pending_x;
        surface->current_y = surface->pending_y;
        surface->has_current_position = true;
        surface->has_pending_position = false;
    }

    /* Apply pending buffer: promote pending to current.
     * The old buffer is released in render_surface() after being
     * rendered. This matches the simple headless compositor model. */

    if (surface->pending_buffer_resource) {
        auto* shm_buf = wl_shm_buffer_get(surface->pending_buffer_resource);
        if (shm_buf) {
            surface->width = wl_shm_buffer_get_width(shm_buf);
            surface->height = wl_shm_buffer_get_height(shm_buf);
        }

        /* Unregister destroy listener from old pending buffer (now becoming current). */
        if (!wl_list_empty(&surface->buffer_destroy_listener.link)) {
            wl_list_remove(&surface->buffer_destroy_listener.link);
        }

        /* Promote pending buffer to current. */
        surface->buffer_resource = surface->pending_buffer_resource;
        surface->buffer_destroyed = false;
        surface->needs_upload = true;

        /* Listen for current buffer destruction. */
        surface->buffer_destroy_listener.notify = buffer_destroy_notify;
        wl_resource_add_destroy_listener(surface->pending_buffer_resource,
                                         &surface->buffer_destroy_listener);
    }
    /* Clear pending — committed (whether null or a buffer). */
    surface->pending_buffer_resource = nullptr;

    /* Clear pending damage. */
    surface->damage_count = 0;

    /* Notify display layer via callback if set (e.g., for xdg-shell configure events). */
    if (surface->commit_callback) {
        surface->commit_callback(resource, surface->commit_callback_user_data);
    }
}

static void surface_frame(struct wl_client* client, struct wl_resource* resource,
                          uint32_t callback_id) {
    (void)client;
    auto* surface = static_cast<MansionSurface*>(wl_resource_get_user_data(resource));

    auto* cb = wl_resource_create(client, &wl_callback_interface, 1, callback_id);
    if (!cb) {
        wl_client_post_no_memory(client);
        return;
    }
    wl_resource_set_destructor(cb, frame_callback_destroyed);
    wl_list_insert(&surface->frame_callback_list, wl_resource_get_link(cb));
}

static void surface_set_opaque_region(struct wl_client* client, struct wl_resource* resource,
                                      struct wl_resource* region) {
    (void)client; (void)resource; (void)region;
}

static void surface_set_input_region(struct wl_client* client, struct wl_resource* resource,
                                      struct wl_resource* region) {
    (void)client; (void)resource; (void)region;
}

static void surface_set_buffer_transform(struct wl_client* client, struct wl_resource* resource,
                                         int32_t transform) {
    (void)client; (void)resource; (void)transform;
}

static void surface_set_buffer_scale(struct wl_client* client, struct wl_resource* resource,
                                     int32_t scale) {
    (void)client; (void)resource; (void)scale;
}

static void surface_damage_buffer(struct wl_client* client, struct wl_resource* resource,
                                  int32_t x, int32_t y, int32_t width, int32_t height) {
    (void)client; (void)resource; (void)x; (void)y; (void)width; (void)height;
}

static const struct wl_surface_interface surface_impl = {
    surface_destroy,
    surface_attach,
    surface_damage,
    surface_frame,
    surface_set_opaque_region,
    surface_set_input_region,
    surface_commit,
    surface_set_buffer_transform,
    surface_set_buffer_scale,
    surface_damage_buffer,
    nullptr, /* offset (v5) */
    nullptr, /* get_release (v7) */
};

/* ---------- wl_region ---------- */

static void region_destroy(struct wl_client* client, struct wl_resource* resource) {
    (void)client;
    wl_resource_destroy(resource);
}

static void region_add(struct wl_client* client, struct wl_resource* resource,
                       int32_t x, int32_t y, int32_t width, int32_t height) {
    (void)client; (void)resource; (void)x; (void)y; (void)width; (void)height;
}

static void region_subtract(struct wl_client* client, struct wl_resource* resource,
                            int32_t x, int32_t y, int32_t width, int32_t height) {
    (void)client; (void)resource; (void)x; (void)y; (void)width; (void)height;
}

static const struct wl_region_interface region_impl = {
    region_destroy,
    region_add,
    region_subtract,
};

/* ---------- compositor ---------- */

static void compositor_create_surface(struct wl_client* client, struct wl_resource* compositor_resource,
                                       uint32_t id) {
    auto* compositor = static_cast<MansionCompositor*>(wl_resource_get_user_data(compositor_resource));

    auto* surface = new (std::nothrow) MansionSurface{};
    if (!surface) {
        wl_client_post_no_memory(client);
        return;
    }
    surface->resource = wl_resource_create(client, &wl_surface_interface, wl_resource_get_version(compositor_resource), id);
    if (!surface->resource) {
        delete surface;
        wl_client_post_no_memory(client);
        return;
    }

    wl_resource_set_implementation(surface->resource, &surface_impl, surface, surface_destroy_callback);

    wl_list_init(&surface->link);
    wl_list_insert(&compositor->surface_list, &surface->link);

    surface->compositor = compositor;

    surface->xdg_surface = nullptr;
    surface->buffer_resource = nullptr;
    surface->pending_buffer_resource = nullptr;
    wl_list_init(&surface->buffer_destroy_listener.link);
    surface->buffer_destroyed = false;
    /* GL texture is now owned by the renderer layer */
    surface->has_pending_position = false;
    surface->has_current_position = false;
    surface->width = 0;
    surface->height = 0;
    surface->needs_upload = false;
    surface->damage_count = 0;
    surface->pending_x_damage = 0;
    surface->pending_y_damage = 0;
    surface->pending_w_damage = 0;
    surface->pending_h_damage = 0;
    wl_list_init(&surface->frame_callback_list);
    surface->commit_callback = nullptr;
    surface->commit_callback_user_data = nullptr;
}

static void compositor_create_region(struct wl_client* client, struct wl_resource* compositor_resource,
                                       uint32_t id) {
    auto* compositor = static_cast<MansionCompositor*>(wl_resource_get_user_data(compositor_resource));
    (void)compositor;

    auto* region = wl_resource_create(client, &wl_region_interface, 1, id);
    if (!region) {
        wl_client_post_no_memory(client);
        return;
    }
    wl_resource_set_implementation(region, &region_impl, nullptr, nullptr);
}

static void compositor_release(struct wl_client* client, struct wl_resource* resource) {
    (void)client;
    wl_resource_destroy(resource);
}

static const struct wl_compositor_interface compositor_impl = {
    compositor_create_surface,
    compositor_create_region,
    compositor_release,
};

static void compositor_bind(struct wl_client* client, void* data, uint32_t version, uint32_t id) {
    auto* compositor = static_cast<MansionCompositor*>(data);
    auto* resource = wl_resource_create(client, &wl_compositor_interface, std::min(version, 4u), id);
    if (!resource) {
        wl_client_post_no_memory(client);
        return;
    }
    wl_resource_set_implementation(resource, &compositor_impl, compositor, nullptr);
}

/* ---------- Core API implementation ---------- */

struct MansionCompositor* compositor_core_create(struct wl_display* display) {
    if (!display) return nullptr;
    auto* compositor = new (std::nothrow) MansionCompositor{};
    if (!compositor) return nullptr;
    wl_list_init(&compositor->surface_list);
    wl_list_init(&compositor->orphaned_surfaces);
    wl_list_init(&compositor->toplevel_list);
    compositor->toplevel_count = 0;
    compositor->focused_surface_resource = nullptr;
    compositor->keyboard_focus_serial = 0;
    compositor->seat = nullptr;

    compositor->global = wl_global_create(display, &wl_compositor_interface, 4,
                                           compositor, compositor_bind);
    if (!compositor->global) {
        delete compositor;
        return nullptr;
    }

    return compositor;
}

void compositor_core_destroy(struct MansionCompositor* compositor) {
    if (!compositor) return;
    wl_global_destroy(compositor->global);

    struct MansionSurface *surface, *next;
    wl_list_for_each_safe(surface, next, &compositor->surface_list, link) {
        wl_resource_destroy(surface->resource);
    }

    /* Clean up orphaned surfaces (surfaces that survived client disconnect). */
    wl_list_for_each_safe(surface, next, &compositor->orphaned_surfaces, link) {
        wl_list_remove(&surface->link);
        delete surface;
    }

    delete compositor;
}

struct wl_display* compositor_core_get_display(struct MansionCompositor* compositor) {
    if (!compositor) return nullptr;
    return wl_global_get_display(compositor->global);
}

struct wl_list* compositor_core_get_surface_list(struct MansionCompositor* compositor) {
    if (!compositor) return nullptr;
    return &compositor->surface_list;
}

struct wl_list* compositor_core_get_toplevel_list(struct MansionCompositor* compositor) {
    if (!compositor) return nullptr;
    return &compositor->toplevel_list;
}

int compositor_core_get_toplevel_count(struct MansionCompositor* compositor) {
    if (!compositor) return 0;
    return compositor->toplevel_count;
}

struct wl_resource* compositor_core_get_focused_surface(struct MansionCompositor* compositor) {
    if (!compositor) return nullptr;
    return compositor->focused_surface_resource;
}

uint32_t compositor_core_get_focus_serial(struct MansionCompositor* compositor) {
    if (!compositor) return 0;
    return compositor->keyboard_focus_serial;
}

void compositor_core_set_focus(struct MansionCompositor* compositor,
                               struct wl_resource* surface,
                               uint32_t serial) {
    if (!compositor) return;
    compositor->focused_surface_resource = surface;
    compositor->keyboard_focus_serial = serial;
}

void compositor_core_clear_focus(struct MansionCompositor* compositor) {
    if (!compositor) return;
    compositor->focused_surface_resource = nullptr;
    compositor->keyboard_focus_serial = 0;
}

void compositor_core_connect_seat(struct MansionCompositor* compositor,
                                   struct MansionSeat* seat) {
    if (!compositor) return;
    compositor->seat = seat;
}

struct wl_resource* compositor_core_surface_first(struct wl_list* surface_list) {
    if (!surface_list || wl_list_empty(surface_list)) return nullptr;
    MansionSurface* surface = wl_container_of(surface_list->next, surface, link);
    return surface->resource;
}

struct wl_resource* compositor_core_surface_next(struct wl_resource* resource) {
    if (!resource) return nullptr;
    auto* surface = static_cast<MansionSurface*>(wl_resource_get_user_data(resource));
    if (!surface || surface->link.next == &surface->compositor->surface_list) return nullptr;
    MansionSurface* next = wl_container_of(surface->link.next, next, link);
    return next->resource;
}

void* compositor_core_surface_get_user_data(struct wl_resource* resource) {
    return wl_resource_get_user_data(resource);
}

void compositor_core_surface_set_user_data(struct wl_resource* resource,
                                           void* user_data) {
    wl_resource_set_user_data(resource, user_data);
}

struct wl_resource* compositor_core_create_surface(struct MansionCompositor* compositor,
                                                   struct wl_client* client,
                                                   uint32_t id) {
    if (!compositor || !client) return nullptr;

    auto* surface = new (std::nothrow) MansionSurface{};
    if (!surface) {
        wl_client_post_no_memory(client);
        return nullptr;
    }
    surface->resource = wl_resource_create(client, &wl_surface_interface, 4, id);
    if (!surface->resource) {
        delete surface;
        wl_client_post_no_memory(client);
        return nullptr;
    }

    wl_resource_set_implementation(surface->resource, &surface_impl, surface, surface_destroy_callback);

    wl_list_init(&surface->link);
    wl_list_insert(&compositor->surface_list, &surface->link);

    surface->compositor = compositor;

    surface->xdg_surface = nullptr;
    surface->buffer_resource = nullptr;
    surface->pending_buffer_resource = nullptr;
    wl_list_init(&surface->buffer_destroy_listener.link);
    surface->buffer_destroyed = false;
    surface->has_pending_position = false;
    surface->has_current_position = false;
    surface->width = 0;
    surface->height = 0;
    surface->needs_upload = false;
    surface->damage_count = 0;
    surface->pending_x_damage = 0;
    surface->pending_y_damage = 0;
    surface->pending_w_damage = 0;
    surface->pending_h_damage = 0;
    wl_list_init(&surface->frame_callback_list);
    surface->commit_callback = nullptr;
    surface->commit_callback_user_data = nullptr;

    return surface->resource;
}

void compositor_core_destroy_surface(struct wl_resource* surface_resource) {
    if (!surface_resource) return;
    wl_resource_destroy(surface_resource);
}

int compositor_core_surface_list_empty(struct MansionCompositor* compositor) {
    if (!compositor) return 1;
    return wl_list_empty(&compositor->surface_list);
}

int compositor_core_orphaned_surfaces_empty(struct MansionCompositor* compositor) {
    if (!compositor) return 1;
    return wl_list_empty(&compositor->orphaned_surfaces);
}

int compositor_core_toplevel_list_empty(struct MansionCompositor* compositor) {
    if (!compositor) return 1;
    return wl_list_empty(&compositor->toplevel_list);
}

void compositor_core_surface_set_commit_callback(struct wl_resource* surface_resource,
                                                  surface_commit_callback callback,
                                                  void* user_data) {
    auto* surface = static_cast<MansionSurface*>(wl_resource_get_user_data(surface_resource));
    if (surface) {
        surface->commit_callback = callback;
        surface->commit_callback_user_data = user_data;
    }
}
