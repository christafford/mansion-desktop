#include <iostream>

#include <wayland-server-protocol.h>
#include <wayland-util.h>

#include "compositor.h"
#include "compositor-private.h"
#include "display.h"

static void surface_destroy_callback(struct wl_resource* resource) {
    auto* surface = static_cast<MansionSurface*>(wl_resource_get_user_data(resource));
    wl_list_remove(&surface->link);
    delete surface;
}

static void surface_attach(struct wl_client* client, struct wl_resource* resource,
                           struct wl_resource* buffer_resource, int32_t x, int32_t y) {
    (void)client; (void)x; (void)y;
    auto* surface = static_cast<MansionSurface*>(wl_resource_get_user_data(resource));

    if (buffer_resource == nullptr) {
        surface->buffer_resource = nullptr;
        surface->buffer_destroyed = false;
        return;
    }

    surface->buffer_resource = buffer_resource;
    surface->buffer_destroyed = false;
}

static void surface_damage(struct wl_client* client, struct wl_resource* resource,
                           int32_t x, int32_t y, int32_t width, int32_t height) {
    (void)client; (void)resource; (void)x; (void)y; (void)width; (void)height;
}

static void surface_commit(struct wl_client* client, struct wl_resource* resource) {
    (void)client;
    auto* surface = static_cast<MansionSurface*>(wl_resource_get_user_data(resource));

    if (surface->has_pending_position) {
        surface->current_x = surface->pending_x;
        surface->current_y = surface->pending_y;
        surface->has_current_position = true;
        surface->has_pending_position = false;
    }

    surface->buffer_destroyed = false;
}

static void surface_frame(struct wl_client* client, struct wl_resource* resource,
                          uint32_t callback) {
    (void)client; (void)resource; (void)callback;
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
    nullptr, /* destroy (v1) - clients rarely call wl_surface_destroy */
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

    surface->buffer_resource = nullptr;
    surface->buffer_destroyed = false;
    surface->has_pending_position = false;
    surface->has_current_position = false;
    surface->width = 0;
    surface->height = 0;
}

static void compositor_create_region(struct wl_client* client, struct wl_resource* compositor_resource,
                                      uint32_t id) {
    (void)client; (void)compositor_resource; (void)id;
    wl_resource_post_error(compositor_resource, WL_DISPLAY_ERROR_INVALID_OBJECT,
                           "region not implemented");
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
    auto* compositor = new MansionCompositor;
    wl_list_init(&compositor->surface_list);

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

    delete compositor;
}
