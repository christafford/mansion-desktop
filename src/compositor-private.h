#pragma once

#include <wayland-server.h>

/* Internal surface structure - shared between compositor.cpp and display.cpp */
struct MansionSurface {
    struct wl_resource* resource;
    struct wl_list link;

    struct wl_resource* buffer_resource;
    int32_t pending_x, pending_y;
    bool has_pending_position;
    int32_t current_x, current_y;
    bool has_current_position;
    bool buffer_destroyed;
};

/* Internal compositor structure - shared between compositor.cpp and display.cpp */
struct MansionCompositor {
    struct wl_global* global;
    struct wl_list surface_list;
};
