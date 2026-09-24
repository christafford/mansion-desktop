#include "compositor.h"

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>

struct MansionCompositor {
    struct wl_global* global;
};

static void destroy_surface(struct wl_resource* resource) {
    wl_resource_destroy(resource);
}

static const struct wl_surface_interface surface_implementation = {
    nullptr, // commit
    nullptr, // set_attached
    nullptr, // set_damaged
    nullptr, // timestamp
    nullptr, // offset
    nullptr, // set_opaque_regions
    nullptr, // set_input_region
    nullptr, // set_generic
    nullptr, // set_buffer_scale
    nullptr, // damage_buffer
};

static void surface_destroy(struct wl_resource* resource) {
    wl_resource_destroy(resource);
}

static void create_surface(struct wl_client* client, struct wl_resource* compositor_resource, uint32_t id) {
    uint32_t version = wl_resource_get_version(compositor_resource);
    struct wl_resource* surface_resource = wl_resource_create(client, &wl_surface_interface, version, id);
    if (!surface_resource) {
        wl_resource_post_no_memory(compositor_resource);
        return;
    }

    wl_resource_set_implementation(surface_resource, &surface_implementation, nullptr, &destroy_surface);
    wl_resource_set_dispatcher(surface_resource, surface_destroy, nullptr, nullptr);
}

static void create_region(struct wl_client* client, struct wl_resource* compositor_resource, uint32_t id, uint32_t version) {
    struct wl_resource* region_resource = wl_resource_create(client, &wl_region_interface, version, id);
    if (!region_resource) {
        wl_resource_post_no_memory(compositor_resource);
        return;
    }
    wl_resource_set_implementation(region_resource, nullptr, nullptr, nullptr);
}

struct MansionCompositor* create_compositor(struct wl_display* display) {
    struct MansionCompositor* compositor = new MansionCompositor;
    if (!compositor) {
        return nullptr;
    }

    static const struct wl_compositor_interface compositor_implementation = {
        create_surface,
        create_region
    };

    compositor->global = wl_global_create(display, &wl_compositor_interface, 4, compositor, &bind_compositor);
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

static void bind_compositor(struct wl_resource* resource, void* data, struct wl_client* client, uint32_t version) {
    (void)data;
    (void)client;
    if (version > 4) {
        wl_resource_post_error(resource, WL_DISPLAY_ERROR_INVALID_GLOBAL, "wl_compositor version %u exceeds 4", version);
        return;
    }

    wl_resource_set_implementation(resource, &wl_compositor_interface, data);
}

static void destroy_compositor_global(struct wl_resource* resource) {
    struct MansionCompositor* compositor = static_cast<struct MansionCompositor*>(wl_resource_get_user_data(resource));
    destroy_compositor(compositor);
}

static const struct wl_global_interface compositor_interface = {
    bind_compositor,
    nullptr
};
