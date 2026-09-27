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

static void surface_destroy_callback(struct wl_resource* resource) {
    auto* surface = static_cast<MansionSurface*>(wl_resource_get_user_data(resource));
    if (!surface) return;

    wl_list_remove(&surface->link);

    /* Fire remaining frame callbacks so clients are not left waiting. */
    struct wl_resource *cb, *cb_next;
    wl_list_for_each_safe(cb, cb_next, &surface->frame_callback_list, link) {
        wl_list_remove(wl_resource_get_link(cb));
        wl_callback_send_done(cb, 0);
        wl_resource_destroy(cb);
    }

    /* Keep MansionSurface alive for screenshot; move to orphaned list.
     * The GL texture and buffer_resource are kept until compositor destruction. */

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

    wl_list_insert(surface->compositor->orphaned_surfaces.prev, &surface->link);
}

static void surface_destroy(struct wl_client* client, struct wl_resource* resource) {
    (void)client; (void)resource;
}

/* Buffer destroy listener — fires when the client destroys the wl_buffer proxy.
 * Clean up the GL texture since the backing storage is gone. */
static void buffer_destroy_notify(struct wl_listener* listener, void* data) {
    (void)data;
    MansionSurface* surface = (MansionSurface*)
        wl_container_of(listener, (MansionSurface*)NULL, buffer_destroy_listener);

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
    auto* surface = static_cast<MansionSurface*>(wl_resource_get_user_data(resource));

    if (buffer_resource == nullptr) {
        surface->buffer_resource = nullptr;
        surface->buffer_destroyed = false;
        /* Clean up the GL texture — no more buffer to render. */
        if (surface->gl_texture) {
            (void)glDeleteTextures(1, &surface->gl_texture);
            surface->gl_texture = 0;
        }
        /* Clear keyboard focus if this surface was focused. */
        if (surface->compositor->focused_surface_resource == surface->resource) {
            seat_set_keyboard_focus(surface->compositor->seat, nullptr, surface->compositor);
        }
        /* Remove any previously registered destroy listener. */
        if (!wl_list_empty(&surface->buffer_destroy_listener.link))
            wl_list_remove(&surface->buffer_destroy_listener.link);
        return;
    }

    /* Remove any previously registered destroy listener. */
    if (!wl_list_empty(&surface->buffer_destroy_listener.link))
        wl_list_remove(&surface->buffer_destroy_listener.link);

    surface->buffer_resource = buffer_resource;
    surface->buffer_destroyed = false;

    /* Listen for buffer destruction. */
    surface->buffer_destroy_listener.notify = buffer_destroy_notify;
    wl_resource_add_destroy_listener(buffer_resource, &surface->buffer_destroy_listener);
}

static void surface_damage(struct wl_client* client, struct wl_resource* resource,
                           int32_t x, int32_t y, int32_t width, int32_t height) {
    (void)client; (void)resource; (void)x; (void)y; (void)width; (void)height;
}

static void surface_commit(struct wl_client* client, struct wl_resource* resource) {
    (void)client;
    auto* surface = static_cast<MansionSurface*>(wl_resource_get_user_data(resource));

    /* Apply pending position. */
    if (surface->has_pending_position) {
        surface->current_x = surface->pending_x;
        surface->current_y = surface->pending_y;
        surface->has_current_position = true;
        surface->has_pending_position = false;
    }

    /* Read buffer size on commit. */
    if (surface->buffer_resource) {
        auto* shm_buf = wl_shm_buffer_get(surface->buffer_resource);
        if (shm_buf) {
            surface->width = wl_shm_buffer_get_width(shm_buf);
            surface->height = wl_shm_buffer_get_height(shm_buf);
        }

        /* Mark texture for re-upload on next render. */
        surface->needs_upload = true;

        /* Give keyboard focus to the most recently mapped toplevel. */
        seat_set_keyboard_focus(surface->compositor->seat,
                                 surface->resource, surface->compositor);
    }

    /* For a simple headless compositor: release the buffer after it has
     * been rendered. We keep buffer_resource set and release in
     * render_surface(). */
    surface->buffer_destroyed = false;

    /* Notify xdg-shell of commit (triggers configure). */
    if (surface->xdg_surface) xdg_shell_on_surface_commit(surface->xdg_surface);
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

    auto* surface = new MansionSurface;
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
    wl_list_init(&surface->buffer_destroy_listener.link);
    surface->buffer_destroyed = false;
    surface->gl_texture = 0;
    surface->has_pending_position = false;
    surface->has_current_position = false;
    surface->width = 0;
    surface->height = 0;
    surface->needs_upload = false;
    wl_list_init(&surface->frame_callback_list);
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
    (void)version;

    auto* resource = wl_resource_create(client, &wl_compositor_interface, 4, id);
    if (!resource) {
        wl_client_post_no_memory(client);
        return;
    }
    wl_resource_set_implementation(resource, &compositor_impl, compositor, nullptr);
}

struct MansionCompositor* create_compositor(struct wl_display* display) {
    auto* compositor = new MansionCompositor{};
    wl_list_init(&compositor->surface_list);
    wl_list_init(&compositor->orphaned_surfaces);
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

struct MansionSurface* compositor_surface_from_resource(struct wl_resource* resource) {
    return static_cast<MansionSurface*>(wl_resource_get_user_data(resource));
}

void compositor_surface_set_size(struct MansionSurface* surface, int32_t width, int32_t height) {
    if (surface) {
        surface->width = width;
        surface->height = height;
    }
}

void destroy_compositor(struct MansionCompositor* compositor) {
    if (!compositor) return;
    wl_global_destroy(compositor->global);

    struct MansionSurface *surface, *next;
    wl_list_for_each_safe(surface, next, &compositor->surface_list, link) {
        wl_resource_destroy(surface->resource);
    }

    /* Clean up orphaned surfaces (surfaces that survived client disconnect). */
    wl_list_for_each_safe(surface, next, &compositor->orphaned_surfaces, link) {
        wl_list_remove(&surface->link);
        if (surface->gl_texture) {
            (void)glDeleteTextures(1, &surface->gl_texture);
            surface->gl_texture = 0;
        }
        delete surface;
    }

    delete compositor;
}
