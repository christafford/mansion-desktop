#include "compositor.h"

#include <wayland-server-core.h>

struct MansionCompositor {
    struct wl_global* global;
};

static void destroy_surface(struct wl_resource* resource) {
    wl_resource_destroy(resource);
}

static void create_surface(struct wl_client* client, struct wl_resource* compositor_resource, uint32_t id, struct wl_resource* surface_resource) {
    surface_resource = wl_resource_create(client, &wl_surface_interface, wl_resource_get_version(compositor_resource), id);
    if (!surface_resource) {
        wl_resource_post_no_memory(compositor_resource);
        return;
    }

    wl_resource_set_implementation(surface_resource, nullptr, nullptr);
    wl_resource_set_destroy_user_data(surface_resource, 1);
    wl_resource_set_dispatcher(surface_resource, nullptr, &destroy_surface);
}

struct MansionCompositor* create_compositor(struct wl_display* display) {
    struct MansionCompositor* compositor = new MansionCompositor;
    if (!compositor) {
        return nullptr;
    }

    compositor->global = wl_global_create(display, &wl_compositor_interface, 4, compositor, bind_compositor);
    if (!compositor->global) {
        free(compositor);
        return nullptr;
    }

    return compositor;
}

void destroy_compositor(struct MansionCompositor* compositor) {
    wl_global_destroy(compositor->global);
    free(compositor);
}

static void bind_compositor(struct wl_resource* resource, void* data, struct wl_resource* client, uint32_t version) {
    (void)data;
    (void)client;
    if (version > 4) {
        wl_resource_post_error(resource, WL_COMPOSITOR_ERROR_INVALID_GLOBAL, "Version %u exceeds %d", version, 4);
        return;
    }

    wl_resource_set_destroy_listener(resource, &destroy_compositor_global);
}

static void destroy_compositor_global(struct wl_resource* resource) {
    struct MansionCompositor* compositor = static_cast<struct MansionCompositor*>(wl_resource_get_user_data(resource));
    destroy_compositor(compositor);
}