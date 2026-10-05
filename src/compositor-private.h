#pragma once

#include <wayland-server.h>

struct MansionXdgSurface;
struct MansionSeat;
struct CoreFrameState;

/* Forward declaration for the toplevel list link. */
struct wl_list;

/* Per-surface frame callback */
struct MansionFrameCallback {
    struct wl_resource* resource;
    struct wl_list link;
};

/* Callback for display layer to intercept surface commits (e.g., for xdg-shell
 * configure events). Set by display layer before surface creation. */
typedef void (*surface_commit_callback)(struct wl_resource* surface_resource,
                                        void* user_data);

/* Internal surface structure - shared between compositor.cpp, display.cpp and xdg-shell.cpp
 *
 * Only Wayland protocol state lives here.  GPU resources (textures) are owned
 * by the rendering layer (renderer-surface.h) and accessed via
 * display.h accessor functions. */
struct MansionSurface {
    CoreFrameState* core_frame = nullptr; // Used only by compositor-core.
    struct wl_resource* resource;
    struct wl_list link;

    /* Pointer back to compositor (needed when resource is destroyed). */
    struct MansionCompositor* compositor;

    /* Role object, owned by its own wl_resource; nullptr without a role. */
    struct MansionXdgSurface* xdg_surface;

    /* Buffer tracking */
    struct wl_resource* buffer_resource;              /* current (applied) buffer */
    struct wl_resource* pending_buffer_resource;       /* buffer set by attach, applied on commit */
    struct wl_listener buffer_destroy_listener;        /* notified when pending_buffer is destroyed */
    bool buffer_destroyed;

    /* Client serial for snapshot lookup */
    uint32_t client_serial;

    /* Surface position (from configure/commit) */
    int32_t pending_x, pending_y;
    bool has_pending_position;
    int32_t current_x, current_y;
    bool has_current_position;

    /* Pending damage (accumulated bounding box since last commit) */
    int pending_x_damage, pending_y_damage;
    int pending_w_damage, pending_h_damage;
    int damage_count;

    /* Surface dimensions (from buffer) */
    int32_t width;
    int32_t height;

    /* True when a new buffer was committed since last upload. */
    bool needs_upload;

    /* Frame callbacks pending fire-on-render */
    struct wl_list frame_callback_list;

    /* Display layer commit callback (for xdg-shell configure events, etc.) */
    surface_commit_callback commit_callback;
    void* commit_callback_user_data;
};

/* Internal compositor structure - shared between compositor.cpp and display.cpp */
struct MansionCompositor {
    struct wl_global* global;
    struct wl_list surface_list;
    struct wl_list orphaned_surfaces; /* surfaces that survived client disconnect */

    /* P5-T01: registry of all toplevel surfaces (across all clients).
     * Each entry is the `link` field inside a MansionXdgSurface that has
     * a non-null toplevel. Iterated left-to-right: oldest toplevel first. */
    struct wl_list toplevel_list;
    int toplevel_count;

    /* Keyboard focus (P1-T06-C). */
    struct MansionSeat* seat;
    struct wl_resource* focused_surface_resource;
    uint32_t keyboard_focus_serial;
};
