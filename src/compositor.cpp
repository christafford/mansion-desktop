#include <cstring>
#include <iostream>

#include <GLES2/gl2.h>
#include <wayland-server-protocol.h>
#include <wayland-util.h>

#include "compositor.h"
#include "compositor-private.h"
#include "display.h"
#include "input.h"
#include "xdg-shell.h"

/* ---------- surface ---------- */

static constexpr int kMaxOrphanedSurfaces = 10;

static void surface_destroy_callback(struct wl_resource* resource) {
    auto* surface = static_cast<ElsewhereSurface*>(wl_resource_get_user_data(resource));
    if (!surface) return;

    wl_list_remove(&surface->link);

    /* Fire remaining frame callbacks so clients are not left waiting. */
    struct wl_resource *cb, *cb_next;
    wl_list_for_each_safe(cb, cb_next, &surface->frame_callback_list, link) {
        wl_list_remove(wl_resource_get_link(cb));
        wl_callback_send_done(cb, 0);
        wl_resource_destroy(cb);
    }

    /* Clear keyboard focus if this surface was the focused one —
     * otherwise focused_surface_resource becomes a dangling pointer. */
    if (surface->compositor->focused_surface_resource == surface->resource) {
        surface->compositor->focused_surface_resource = nullptr;
        seat_clear_focus_state();

        /* P3-T06: If the focused surface disappears while in Application
         * mode, return to World. Do NOT call exit_application_mode() here
         * — it sends Wayland events that may crash when the Wayland library
         * is simultaneously processing the client's resource destruction. */
        if (input_mode_get() == InputMode::Application) {
            input_mode_set(InputMode::World);
        }
    }

    /* Insert at tail (newest last). If the list exceeds the max, evict
     * the oldest surface (first in the list) and free its resources. */
    if (surface->compositor->orphaned_surfaces.next != &surface->compositor->orphaned_surfaces) {
        int count = 0;
        ElsewhereSurface *s;
        wl_list_for_each(s, &surface->compositor->orphaned_surfaces, link) {
            (void)s;
            count++;
        }
        if (count >= kMaxOrphanedSurfaces) {
            ElsewhereSurface* oldest = wl_container_of(
                surface->compositor->orphaned_surfaces.next, oldest, link);
            wl_list_remove(&oldest->link);
            destroy_renderer_surface(oldest);
            delete oldest;
        }
    }

    wl_list_insert(surface->compositor->orphaned_surfaces.prev, &surface->link);
}

static void surface_destroy(struct wl_client* client, struct wl_resource* resource) {
    (void)client; (void)resource;
}

/* Buffer destroy listener — fires when the client destroys the wl_buffer proxy.
 * Clean up the GL texture since the backing storage is gone. */
static void buffer_destroy_notify(struct wl_listener* listener, void* data) {
    (void)data;
    ElsewhereSurface* surface = (ElsewhereSurface*)
        wl_container_of(listener, (ElsewhereSurface*)NULL, buffer_destroy_listener);

    if (!surface) return;
    surface->buffer_destroyed = true;
    surface->buffer_resource = nullptr;

    /* The GL texture is a pixel copy (glTexImage2D) and remains valid
     * for orphaned-surface rendering. We delete it on re-upload in
     * render_panel() where we also create the new texture. */
}

static void surface_attach(struct wl_client* client, struct wl_resource* resource,
                           struct wl_resource* buffer_resource, int32_t x, int32_t y) {
    (void)client; (void)x; (void)y;
    auto* surface = static_cast<ElsewhereSurface*>(wl_resource_get_user_data(resource));

    /* Always remove the listener — it may be registered for the current
     * buffer from a previous commit. We will re-register it for the new
     * pending buffer below (or not at all if the new buffer is null). */
    if (!wl_list_empty(&surface->buffer_destroy_listener.link)) {
        wl_list_remove(&surface->buffer_destroy_listener.link);
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
    auto* surface = static_cast<ElsewhereSurface*>(wl_resource_get_user_data(resource));
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
    auto* surface = static_cast<ElsewhereSurface*>(wl_resource_get_user_data(resource));

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

    /* Give keyboard focus to the most recently mapped toplevel. */
    if (surface->buffer_resource) {
        seat_set_keyboard_focus(surface->compositor->seat,
                                  surface->resource, surface->compositor);
    }

    /* Clear pending damage. */
    surface->damage_count = 0;

    /* Notify xdg-shell of commit (triggers configure). */
    if (surface->xdg_surface && xdg_shell_on_surface_commit(surface->xdg_surface))
        xdg_shell_on_surface_applied(surface->xdg_surface, surface->width, surface->height);
}

static void surface_frame(struct wl_client* client, struct wl_resource* resource,
                          uint32_t callback_id) {
    (void)client;
    auto* surface = static_cast<ElsewhereSurface*>(wl_resource_get_user_data(resource));

    auto* cb = wl_resource_create(client, &wl_callback_interface, 1, callback_id);
    if (!cb) {
        wl_client_post_no_memory(client);
        return;
    }
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
    auto* compositor = static_cast<ElsewhereCompositor*>(wl_resource_get_user_data(compositor_resource));

    auto* surface = new ElsewhereSurface;
    surface->resource = wl_resource_create(client, &wl_surface_interface, 4, id);
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
    /* P21-T10: GL texture is now owned by the renderer layer */
    ensure_renderer_surface(surface);
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
}

static void compositor_create_region(struct wl_client* client, struct wl_resource* compositor_resource,
                                       uint32_t id) {
    auto* compositor = static_cast<ElsewhereCompositor*>(wl_resource_get_user_data(compositor_resource));
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
    auto* compositor = static_cast<ElsewhereCompositor*>(data);
    (void)version;

    auto* resource = wl_resource_create(client, &wl_compositor_interface, 4, id);
    if (!resource) {
        wl_client_post_no_memory(client);
        return;
    }
    wl_resource_set_implementation(resource, &compositor_impl, compositor, nullptr);
}

struct ElsewhereCompositor* create_compositor(struct wl_display* display) {
    auto* compositor = new ElsewhereCompositor{};
    wl_list_init(&compositor->surface_list);
    wl_list_init(&compositor->orphaned_surfaces);
    wl_list_init(&compositor->toplevel_list);
    compositor->toplevel_count = 0;
    compositor->focused_surface_resource = nullptr;
    compositor->keyboard_focus_serial = 0;

    compositor->global = wl_global_create(display, &wl_compositor_interface, 4,
                                           compositor, compositor_bind);
    if (!compositor->global) {
        delete compositor;
        return nullptr;
    }

    return compositor;
}

struct ElsewhereSurface* compositor_surface_from_resource(struct wl_resource* resource) {
    return static_cast<ElsewhereSurface*>(wl_resource_get_user_data(resource));
}

struct ElsewhereSurface* compositor_surface_from_serial(struct ElsewhereCompositor* compositor, uint32_t serial) {
    if (!compositor) return nullptr;

    struct ElsewhereSurface *surface;
    wl_list_for_each(surface, &compositor->surface_list, link) {
        if (surface->client_serial == serial) {
            return surface;
        }
    }

    /* Also check orphaned surfaces */
    wl_list_for_each(surface, &compositor->orphaned_surfaces, link) {
        if (surface->client_serial == serial) {
            return surface;
        }
    }

    return nullptr;
}

void compositor_surface_set_size(struct ElsewhereSurface* surface, int32_t width, int32_t height) {
    if (surface) {
        surface->width = width;
        surface->height = height;
    }
}

void destroy_compositor(struct ElsewhereCompositor* compositor) {
    if (!compositor) return;
    wl_global_destroy(compositor->global);

    struct ElsewhereSurface *surface, *next;
    wl_list_for_each_safe(surface, next, &compositor->surface_list, link) {
        wl_resource_destroy(surface->resource);
    }

    /* Clean up orphaned surfaces (surfaces that survived client disconnect). */
    wl_list_for_each_safe(surface, next, &compositor->orphaned_surfaces, link) {
        wl_list_remove(&surface->link);
        destroy_renderer_surface(surface);
        delete surface;
    }

    delete compositor;
}
