/*
 * ElsewhereGDExtensionAdapter - GDExtension adapter for compositor-core
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
  #define ELSEWHERE_GDEXT_EXPORT __declspec(dllexport)
#else
  #define ELSEWHERE_GDEXT_EXPORT __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Snapshot structure (forward declaration for adapter API) */
struct ElsewhereShmSnapshot {
    int32_t width;
    int32_t height;
    int32_t stride;           /* bytes per row */
    int32_t format;           /* WL_SHM_FORMAT_* constant */
    uint32_t revision;        /* increments on each snapshot */
    uint8_t *pixels;          /* pixel data, ARGB8888 format */
    size_t pixels_size;       /* total bytes in pixels buffer */
};

/* Internal adapter state (defined in implementation file) */
struct ElsewhereGDExtensionAdapter {
    struct wl_display *display;
    struct ElsewhereCompositor *compositor;
    struct wl_event_loop *event_loop;
    int debug_logging;
    int owned_display;         /* True if we created and own the display */
    int owned_compositor;      /* True if we created and own the compositor */
};

/* Configuration for adapter creation */
typedef struct {
    struct wl_display *display;        /* Existing Wayland display (or NULL to create new) */
    struct ElsewhereCompositor *compositor; /* Existing compositor (or NULL to create new) */
    const char *socket_name;           /* Wayland display socket name (only used if display is NULL) */
    int debug_logging;                 /* Enable debug logging */
} ElsewhereGDExtensionConfig;

/* Create a new adapter instance.
 * Returns NULL on failure. */
ELSEWHERE_GDEXT_EXPORT ElsewhereGDExtensionAdapter* elsewhere_gdextension_adapter_create(
    const ElsewhereGDExtensionConfig *config);

/* Destroy the adapter and free all resources. */
ELSEWHERE_GDEXT_EXPORT void elsewhere_gdextension_adapter_destroy(ElsewhereGDExtensionAdapter *adapter);

/* Get the Wayland display associated with the adapter. */
ELSEWHERE_GDEXT_EXPORT struct wl_display* elsewhere_gdextension_adapter_get_display(
    ElsewhereGDExtensionAdapter *adapter);

/* Get the compositor instance. */
ELSEWHERE_GDEXT_EXPORT struct ElsewhereCompositor* elsewhere_gdextension_adapter_get_compositor(
    ElsewhereGDExtensionAdapter *adapter);

/* Pump events from the event loop (non-blocking).
 * Returns number of events processed, or -1 on error. */
ELSEWHERE_GDEXT_EXPORT int elsewhere_gdextension_adapter_pump(ElsewhereGDExtensionAdapter *adapter);

/* Flush pending events to clients.
 * Returns 0 on success, -1 on error. */
ELSEWHERE_GDEXT_EXPORT int elsewhere_gdextension_adapter_flush(ElsewhereGDExtensionAdapter *adapter);

/* Create a new surface.
 * Returns NULL on failure. */
ELSEWHERE_GDEXT_EXPORT struct wl_resource* elsewhere_gdextension_adapter_create_surface(
    ElsewhereGDExtensionAdapter *adapter,
    struct wl_client *client,
    uint32_t id);

/* Destroy a surface. */
ELSEWHERE_GDEXT_EXPORT void elsewhere_gdextension_adapter_destroy_surface(
    ElsewhereGDExtensionAdapter *adapter,
    struct wl_resource *surface);

/* Set keyboard focus on a surface. */
ELSEWHERE_GDEXT_EXPORT void elsewhere_gdextension_adapter_set_focus(
    ElsewhereGDExtensionAdapter *adapter,
    struct wl_resource *surface,
    uint32_t serial);

/* Clear keyboard focus. */
ELSEWHERE_GDEXT_EXPORT void elsewhere_gdextension_adapter_clear_focus(
    ElsewhereGDExtensionAdapter *adapter);

/* Get the currently focused surface, or NULL if none. */
ELSEWHERE_GDEXT_EXPORT struct wl_resource* elsewhere_gdextension_adapter_get_focused_surface(
    ElsewhereGDExtensionAdapter *adapter);

/* Get the focus serial. */
ELSEWHERE_GDEXT_EXPORT uint32_t elsewhere_gdextension_adapter_get_focused_serial(
    ElsewhereGDExtensionAdapter *adapter);

/* ─── Shm frame snapshot API ─── */

/* Create a snapshot of the committed shm buffer pixels for a surface.
 * Returns owned pixel data in ARGB8888 format. Returns NULL if no buffer
 * is committed or copy fails. Caller must free via elsewhere_snapshot_destroy(). */
ELSEWHERE_GDEXT_EXPORT ElsewhereShmSnapshot* elsewhere_gdextension_adapter_create_snapshot(
    ElsewhereGDExtensionAdapter *adapter,
    uint32_t client_serial);

/* Destroy a snapshot and free its pixel buffer. Safe to call with NULL. */
ELSEWHERE_GDEXT_EXPORT void elsewhere_gdextension_adapter_destroy_snapshot(
    ElsewhereShmSnapshot *snapshot);

/* Get snapshot width. */
ELSEWHERE_GDEXT_EXPORT int32_t elsewhere_gdextension_snapshot_get_width(
    ElsewhereShmSnapshot *snapshot);

/* Get snapshot height. */
ELSEWHERE_GDEXT_EXPORT int32_t elsewhere_gdextension_snapshot_get_height(
    ElsewhereShmSnapshot *snapshot);

/* Get snapshot stride (bytes per row). */
ELSEWHERE_GDEXT_EXPORT int32_t elsewhere_gdextension_snapshot_get_stride(
    ElsewhereShmSnapshot *snapshot);

/* Get snapshot format (WL_SHM_FORMAT_*). */
ELSEWHERE_GDEXT_EXPORT int32_t elsewhere_gdextension_snapshot_get_format(
    ElsewhereShmSnapshot *snapshot);

/* Get snapshot revision. */
ELSEWHERE_GDEXT_EXPORT uint32_t elsewhere_gdextension_snapshot_get_revision(
    ElsewhereShmSnapshot *snapshot);

/* Get pointer to pixel data (ARGB8888 format). */
ELSEWHERE_GDEXT_EXPORT uint8_t* elsewhere_gdextension_snapshot_get_pixels(
    ElsewhereShmSnapshot *snapshot);

/* Get total bytes in pixel buffer. */
ELSEWHERE_GDEXT_EXPORT size_t elsewhere_gdextension_snapshot_get_pixels_size(
    ElsewhereShmSnapshot *snapshot);

/* Copy snapshot pixels into a caller-provided buffer. Returns number of bytes copied, or -1 on error. */
ELSEWHERE_GDEXT_EXPORT int elsewhere_gdextension_snapshot_copy_pixels(
    ElsewhereShmSnapshot *snapshot,
    uint8_t *out_buffer,
    size_t buffer_size);

#ifdef __cplusplus
}
#endif
