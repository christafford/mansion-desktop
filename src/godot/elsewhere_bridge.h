#pragma once

/*
 * ElsewhereBridge — C-facing bridge between the GDExtension and the
 * Wayland compositor core.
 *
 * This header provides only C-compatible symbols so the GDExtension
 * entry point (C) can call into the C++ adapter without ABI issues.
 * All memory is owned by the adapter; the caller must not free
 * returned pointers except via the explicit destroy functions.
 *
 * No GL/EGL includes — the bridge only talks to the protocol core.
 */

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ─── Surface info (lightweight snapshot, no raw pointers escape) ─── */

typedef struct ElsewhereSurfaceInfo {
    int32_t width;
    int32_t height;
    int32_t x;
    int32_t y;
    bool needs_upload;
} ElsewhereSurfaceInfo;

/* ─── Shm frame snapshot (owned pixel data for Godot) ───
 *
 * An owned pixel buffer snapshot that survives the lifetime of the
 * original Wayland shm buffer. Pixels are stored in ARGB8888 format
 * (0xAARRGGBB, little-endian) for direct texture upload to Godot.
 *
 * All memory is owned by the adapter; callers must not free pointers
 * except via elsewhere_snapshot_destroy().
 *
 * Note: ElsewhereShmSnapshot is defined in elsewhere_gdextension_adapter.h
 * to avoid circular dependencies between C and C++ code. */

/* ─── Callback for surface discovery ───
 * Called once per surface during enumeration.
 * user_data is forwarded from the caller.
 */
typedef void (*elsewhere_surface_enumerate_fn)(void *user_data,
    uint32_t client_serial,
    const ElsewhereSurfaceInfo *info);

/* ─── Adapter configuration ─── */

typedef struct ElsewhereAdapterConfig {
    const char *socket_name;     /* e.g. "elsewhere0" — NULL = auto */
    uint32_t configure_width;    /* initial xdg configure size */
    uint32_t configure_height;
    bool debug_logging;
} ElsewhereAdapterConfig;

/* ─── Lifecycle ─── */

/* Create the adapter: opens private socket, creates compositor + seat.
 * Returns opaque handle on success, NULL on failure.
 * The adapter owns the socket and compositor for the lifetime of the handle. */
void *elsewhere_adapter_create(const ElsewhereAdapterConfig *config);

/* Destroy the adapter: tears down compositor, closes socket, frees memory.
 * Safe to call with NULL. */
void elsewhere_adapter_destroy(void *handle);

/* ─── Event pump ─── */

/* Non-blocking dispatch of pending events on the compositor's event loop.
 * Returns the number of events dispatched (may be 0).
 * Must be called from the owning thread (the same thread that created
 * the adapter). Does NOT block. */
int elsewhere_adapter_pump(void *handle);

/* Flush pending writes to connected clients.
 * Returns 0 on success, -1 on error. */
int elsewhere_adapter_flush(void *handle);

/* ─── Surface enumeration ─── */

/* Enumerate all known surfaces (mapped and orphaned), calling the
 * provided callback for each. The callback receives a lightweight
 * snapshot (ElsewhereSurfaceInfo) — no raw Wayland pointers escape. */
void elsewhere_adapter_enumerate_surfaces(void *handle,
    elsewhere_surface_enumerate_fn callback,
    void *user_data);

/* ─── Frame callback dispatch ───
 * Fire all pending frame callbacks on surfaces that were rendered.
 * Returns the number of callbacks fired.
 * Must be called after rendering each frame. */
int elsewhere_adapter_fire_frame_callbacks(void *handle);

/* ─── Shm frame snapshot ───
 * Copy committed shm pixels into owned snapshots.
 *
 * elsewhere_adapter_create_snapshot:
 *   Create a snapshot of the current committed buffer for a surface.
 *   Returns owned pixel data in ARGB8888 format (no raw Wayland pointers).
 *   Returns NULL if no buffer is committed or copy fails.
 *
 * elsewhere_adapter_destroy_snapshot:
 *   Free a snapshot and its pixel buffer. Safe to call with NULL. */
ElsewhereShmSnapshot *elsewhere_adapter_create_snapshot(void *handle, uint32_t client_serial);
void elsewhere_adapter_destroy_snapshot(ElsewhereShmSnapshot *snapshot);

/* ─── Client launch ───
 * Launch a client on this adapter's socket. `command` is a
 * whitespace-separated string (e.g. "weston-terminal").
 * Returns the child PID on success, -1 on fork/exec failure. */
int32_t elsewhere_adapter_launch_client(void *handle, const char *command);

/* ─── Seat configuration ─── */

/* Set the keyboard modifier state (for GDScript to query).
 * bitfield: bit 0 = shift, bit 1 = control, bit 2 = alt, bit 3 = meta */
void elsewhere_adapter_set_modifiers(void *handle, uint32_t mods);

/* Get the currently focused surface's client serial (for GDScript queries). */
uint32_t elsewhere_adapter_get_focused_serial(void *handle);

/* ─── Shutdown ─── */

/* Request clean shutdown. After this returns the compositor core
 * should be torn down (call elsewhere_adapter_destroy). */
void elsewhere_adapter_shutdown(void *handle);

#ifdef __cplusplus
}
#endif
