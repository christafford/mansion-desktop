/*
 * MansionGDExtensionAdapter - GDExtension adapter for compositor-core
 *
 * This header defines the C API that the GDExtension library exposes to Godot.
 * It wraps the compositor-core library and provides a minimal interface for
 * creating surfaces and pumping events.
 */

#pragma once

#include <stdint.h>
#include <wayland-server.h>

/* Visibility macro for shared library exports */
#ifdef _WIN32
  #define MANSION_GDEXT_EXPORT __declspec(dllexport)
#else
  #define MANSION_GDEXT_EXPORT __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Snapshot structure (forward declaration for adapter API) */
struct MansionShmSnapshot {
    int32_t width;
    int32_t height;
    int32_t stride;           /* bytes per row */
    int32_t format;           /* WL_SHM_FORMAT_* constant */
    uint32_t revision;        /* increments on each snapshot */
    uint8_t *pixels;          /* pixel data, ARGB8888 format */
    size_t pixels_size;       /* total bytes in pixels buffer */
};

/* Internal adapter state (defined in implementation file) */
struct MansionGDExtensionAdapter {
    struct wl_display *display;
    struct MansionCompositor *compositor;
    struct wl_event_loop *event_loop;
    int debug_logging;
    int owned_display;         /* True if we created and own the display */
    int owned_compositor;      /* True if we created and own the compositor */
};

/* Configuration for adapter creation */
typedef struct {
    struct wl_display *display;        /* Existing Wayland display (or NULL to create new) */
    struct MansionCompositor *compositor; /* Existing compositor (or NULL to create new) */
    const char *socket_name;           /* Wayland display socket name (only used if display is NULL) */
    int debug_logging;                 /* Enable debug logging */
} MansionGDExtensionConfig;

/* Create a new adapter instance.
 * Returns NULL on failure. */
MANSION_GDEXT_EXPORT MansionGDExtensionAdapter* mansion_gdextension_adapter_create(
    const MansionGDExtensionConfig *config);

/* Destroy the adapter and free all resources. */
MANSION_GDEXT_EXPORT void mansion_gdextension_adapter_destroy(MansionGDExtensionAdapter *adapter);

/* Get the Wayland display associated with the adapter. */
MANSION_GDEXT_EXPORT struct wl_display* mansion_gdextension_adapter_get_display(
    MansionGDExtensionAdapter *adapter);

/* Get the compositor instance. */
MANSION_GDEXT_EXPORT struct MansionCompositor* mansion_gdextension_adapter_get_compositor(
    MansionGDExtensionAdapter *adapter);

/* Pump events from the event loop (non-blocking).
 * Returns number of events processed, or -1 on error. */
MANSION_GDEXT_EXPORT int mansion_gdextension_adapter_pump(MansionGDExtensionAdapter *adapter);

/* Flush pending events to clients.
 * Returns 0 on success, -1 on error. */
MANSION_GDEXT_EXPORT int mansion_gdextension_adapter_flush(MansionGDExtensionAdapter *adapter);

/* Create a new surface.
 * Returns NULL on failure. */
MANSION_GDEXT_EXPORT struct wl_resource* mansion_gdextension_adapter_create_surface(
    MansionGDExtensionAdapter *adapter,
    struct wl_client *client,
    uint32_t id);

/* Destroy a surface. */
MANSION_GDEXT_EXPORT void mansion_gdextension_adapter_destroy_surface(
    MansionGDExtensionAdapter *adapter,
    struct wl_resource *surface);

/* Set keyboard focus on a surface. */
MANSION_GDEXT_EXPORT void mansion_gdextension_adapter_set_focus(
    MansionGDExtensionAdapter *adapter,
    struct wl_resource *surface,
    uint32_t serial);

/* Clear keyboard focus. */
MANSION_GDEXT_EXPORT void mansion_gdextension_adapter_clear_focus(
    MansionGDExtensionAdapter *adapter);

/* Get the currently focused surface, or NULL if none. */
MANSION_GDEXT_EXPORT struct wl_resource* mansion_gdextension_adapter_get_focused_surface(
    MansionGDExtensionAdapter *adapter);

/* Get the focus serial. */
MANSION_GDEXT_EXPORT uint32_t mansion_gdextension_adapter_get_focused_serial(
    MansionGDExtensionAdapter *adapter);

/* ─── Shm frame snapshot API ─── */

/* Create a snapshot of the committed shm buffer pixels for a surface.
 * Returns owned pixel data in ARGB8888 format. Returns NULL if no buffer
 * is committed or copy fails. Caller must free via mansion_snapshot_destroy(). */
MANSION_GDEXT_EXPORT MansionShmSnapshot* mansion_gdextension_adapter_create_snapshot(
    MansionGDExtensionAdapter *adapter,
    uint32_t client_serial);

/* Destroy a snapshot and free its pixel buffer. Safe to call with NULL. */
MANSION_GDEXT_EXPORT void mansion_gdextension_adapter_destroy_snapshot(
    MansionShmSnapshot *snapshot);

/* Get snapshot width. */
MANSION_GDEXT_EXPORT int32_t mansion_gdextension_snapshot_get_width(
    MansionShmSnapshot *snapshot);

/* Get snapshot height. */
MANSION_GDEXT_EXPORT int32_t mansion_gdextension_snapshot_get_height(
    MansionShmSnapshot *snapshot);

/* Get snapshot stride (bytes per row). */
MANSION_GDEXT_EXPORT int32_t mansion_gdextension_snapshot_get_stride(
    MansionShmSnapshot *snapshot);

/* Get snapshot format (WL_SHM_FORMAT_*). */
MANSION_GDEXT_EXPORT int32_t mansion_gdextension_snapshot_get_format(
    MansionShmSnapshot *snapshot);

/* Get snapshot revision. */
MANSION_GDEXT_EXPORT uint32_t mansion_gdextension_snapshot_get_revision(
    MansionShmSnapshot *snapshot);

/* Get pointer to pixel data (ARGB8888 format). */
MANSION_GDEXT_EXPORT uint8_t* mansion_gdextension_snapshot_get_pixels(
    MansionShmSnapshot *snapshot);

/* Get total bytes in pixel buffer. */
MANSION_GDEXT_EXPORT size_t mansion_gdextension_snapshot_get_pixels_size(
    MansionShmSnapshot *snapshot);

/* Copy snapshot pixels into a caller-provided buffer. Returns number of bytes copied, or -1 on error. */
MANSION_GDEXT_EXPORT int mansion_gdextension_snapshot_copy_pixels(
    MansionShmSnapshot *snapshot,
    uint8_t *out_buffer,
    size_t buffer_size);

#ifdef __cplusplus
}
#endif
