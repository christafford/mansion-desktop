#pragma once

#include <GLES2/gl2.h>
#include <wayland-server.h>

struct MansionXdgSurface;
struct MansionSeat;

/* Per-surface frame callback */
struct MansionFrameCallback {
    struct wl_resource* resource;
    struct wl_list link;
};

/* Internal surface structure - shared between compositor.cpp, display.cpp and xdg-shell.cpp */
struct MansionSurface {
    struct wl_resource* resource;
    struct wl_list link;

    /* Pointer back to compositor (needed when resource is destroyed). */
    struct MansionCompositor* compositor;

    /* Role object, owned by its own wl_resource; nullptr without a role. */
    struct MansionXdgSurface* xdg_surface;

    /* Buffer tracking */
    struct wl_resource* buffer_resource;
    struct wl_listener buffer_destroy_listener; /* notified when buffer_resource is destroyed */
    bool buffer_destroyed;

    /* GL texture for the current buffer (0 = no texture) */
    GLuint gl_texture;

    int32_t pending_x, pending_y;
    bool has_pending_position;
    int32_t current_x, current_y;
    bool has_current_position;

    /* Surface dimensions (from buffer) */
    int32_t width;
    int32_t height;

    /* True when a new buffer was committed since last upload. */
    bool needs_upload;

    /* Frame callbacks pending fire-on-render */
    struct wl_list frame_callback_list;
};

/* Internal compositor structure - shared between compositor.cpp and display.cpp */
struct MansionCompositor {
    struct wl_global* global;
    struct wl_list surface_list;
    struct wl_list orphaned_surfaces; /* surfaces that survived client disconnect */

    /* Keyboard focus (P1-T06-C). */
    struct MansionSeat* seat;
    struct wl_resource* focused_surface_resource;
    uint32_t keyboard_focus_serial;
};
