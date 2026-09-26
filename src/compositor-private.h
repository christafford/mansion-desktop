#pragma once

#include <wayland-server.h>

struct MansionXdgSurface;

/* Per-surface frame callback */
struct MansionFrameCallback {
    struct wl_resource* resource;
    struct wl_list link;
};

/* Internal surface structure - shared between compositor.cpp, display.cpp and xdg-shell.cpp */
struct MansionSurface {
    struct wl_resource* resource;
    struct wl_list link;

    /* Role object, owned by its own wl_resource; nullptr without a role. */
    struct MansionXdgSurface* xdg_surface;

    /* Buffer tracking */
    struct wl_resource* buffer_resource;
    struct wl_listener buffer_destroy_listener; /* notified when buffer_resource is destroyed */
    bool buffer_destroyed;

    int32_t pending_x, pending_y;
    bool has_pending_position;
    int32_t current_x, current_y;
    bool has_current_position;

    /* Surface dimensions (from buffer) */
    int32_t width;
    int32_t height;

    /* Frame callbacks pending fire-on-render */
    struct wl_list frame_callback_list;
};

/* Internal compositor structure - shared between compositor.cpp and display.cpp */
struct MansionCompositor {
    struct wl_global* global;
    struct wl_list surface_list;
};
