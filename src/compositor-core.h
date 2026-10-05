#pragma once

#include <wayland-server.h>

#ifdef __cplusplus
extern "C" {
#endif

struct MansionCompositor;
struct MansionSeat;

/* ---------- Core library API ---------- */

/* Create a new compositor instance.
 * Returns nullptr on failure. */
struct MansionCompositor* compositor_core_create(struct wl_display* display);

/* Destroy the compositor and free all resources.
 * Destroy its clients first; bound protocol resources refer to this core. */
void compositor_core_destroy(struct MansionCompositor* compositor);

/* Get the Wayland display associated with the compositor. */
struct wl_display* compositor_core_get_display(struct MansionCompositor* compositor);

/* Get the surface list head for iteration. */
struct wl_list* compositor_core_get_surface_list(struct MansionCompositor* compositor);

/* Get the toplevel list head for iteration. */
struct wl_list* compositor_core_get_toplevel_list(struct MansionCompositor* compositor);

/* Get the toplevel count. */
int compositor_core_get_toplevel_count(struct MansionCompositor* compositor);

/* Get the focused surface resource, or nullptr if none. */
struct wl_resource* compositor_core_get_focused_surface(struct MansionCompositor* compositor);

/* Get the keyboard focus serial. */
uint32_t compositor_core_get_focus_serial(struct MansionCompositor* compositor);

/* Check if surface list is empty. */
int compositor_core_surface_list_empty(struct MansionCompositor* compositor);

/* Find a surface by its client serial. */
struct MansionSurface* compositor_surface_from_serial(struct MansionCompositor* compositor, uint32_t serial);

/* Check if orphaned surfaces list is empty. */
int compositor_core_orphaned_surfaces_empty(struct MansionCompositor* compositor);

/* Check if toplevel list is empty. */
int compositor_core_toplevel_list_empty(struct MansionCompositor* compositor);

/* Set keyboard focus (core layer only - does not send Wayland events).
 * Display layer must handle seat focus notifications separately. */
void compositor_core_set_focus(struct MansionCompositor* compositor,
                               struct wl_resource* surface,
                               uint32_t serial);

/* Clear focus state (call when focused surface is destroyed).
 * Display layer must handle seat focus cleanup separately. */
void compositor_core_clear_focus(struct MansionCompositor* compositor);

/* Connect seat to compositor (called by display layer after seat creation). */
void compositor_core_connect_seat(struct MansionCompositor* compositor,
                                   struct MansionSeat* seat);

/* Complete only committed callbacks after the runtime consumes the dispatch.
 * This is CPU bridge pacing, not evidence of display scanout/presentation. */
void compositor_core_complete_frames(struct MansionCompositor* compositor);

/* ---------- Surface iteration helpers ---------- */

/* Get the first surface in the list, or nullptr if empty. */
struct wl_resource* compositor_core_surface_first(struct wl_list* surface_list);

/* Get the next surface in the list, or nullptr if at end. */
struct wl_resource* compositor_core_surface_next(struct wl_resource* resource);

/* Get the user data (MansionSurface*) from a surface resource. */
void* compositor_core_surface_get_user_data(struct wl_resource* resource);

/* Set the user data for a surface resource. */
void compositor_core_surface_set_user_data(struct wl_resource* resource,
                                           void* user_data);

/* ---------- Surface commit callback ---------- */

/* Signature for commit callbacks.
 * Called after each wl_surface.commit with the surface resource.
 * Display layer uses this for xdg-shell configure events, etc. */
typedef void (*surface_commit_callback)(struct wl_resource* surface_resource,
                                        void* user_data);

/* ---------- Surface lifecycle (core layer) ---------- */

/* Create a new surface with only core protocol state.
 * Returns nullptr on failure (client must be from wl_compositor interface handler). */
struct wl_resource* compositor_core_create_surface(struct MansionCompositor* compositor,
                                                   struct wl_client* client,
                                                   uint32_t id);

/* Destroy a surface and its protocol state; owned snapshots must be separate. */
void compositor_core_destroy_surface(struct wl_resource* surface_resource);

/* Set the commit callback for a surface.
 * Called after each wl_surface.commit with the surface resource.
 * Display layer uses this for xdg-shell configure events, etc. */
void compositor_core_surface_set_commit_callback(struct wl_resource* surface_resource,
                                                  surface_commit_callback callback,
                                                  void* user_data);

#ifdef __cplusplus
}
#endif
