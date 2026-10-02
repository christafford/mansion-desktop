#pragma once

/*
 * MansionBridge — C-facing bridge between the GDExtension and the
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

typedef struct MansionSurfaceInfo {
    int32_t width;
    int32_t height;
    int32_t x;
    int32_t y;
    bool needs_upload;
} MansionSurfaceInfo;

/* ─── Callback for surface discovery ───
 * Called once per surface during enumeration.
 * user_data is forwarded from the caller.
 */
typedef void (*mansion_surface_enumerate_fn)(void *user_data,
    uint32_t client_serial,
    const MansionSurfaceInfo *info);

/* ─── Adapter configuration ─── */

typedef struct MansionAdapterConfig {
    const char *socket_name;     /* e.g. "mansion0" — NULL = auto */
    uint32_t configure_width;    /* initial xdg configure size */
    uint32_t configure_height;
    bool debug_logging;
} MansionAdapterConfig;

/* ─── Lifecycle ─── */

/* Create the adapter: opens private socket, creates compositor + seat.
 * Returns opaque handle on success, NULL on failure.
 * The adapter owns the socket and compositor for the lifetime of the handle. */
void *mansion_adapter_create(const MansionAdapterConfig *config);

/* Destroy the adapter: tears down compositor, closes socket, frees memory.
 * Safe to call with NULL. */
void mansion_adapter_destroy(void *handle);

/* ─── Event pump ─── */

/* Non-blocking dispatch of pending events on the compositor's event loop.
 * Returns the number of events dispatched (may be 0).
 * Must be called from the owning thread (the same thread that created
 * the adapter). Does NOT block. */
int mansion_adapter_pump(void *handle);

/* Flush pending writes to connected clients.
 * Returns 0 on success, -1 on error. */
int mansion_adapter_flush(void *handle);

/* ─── Surface enumeration ─── */

/* Enumerate all known surfaces (mapped and orphaned), calling the
 * provided callback for each. The callback receives a lightweight
 * snapshot (MansionSurfaceInfo) — no raw Wayland pointers escape. */
void mansion_adapter_enumerate_surfaces(void *handle,
    mansion_surface_enumerate_fn callback,
    void *user_data);

/* ─── Frame callback dispatch ───
 * Fire all pending frame callbacks on surfaces that were rendered.
 * Returns the number of callbacks fired.
 * Must be called after rendering each frame. */
int mansion_adapter_fire_frame_callbacks(void *handle);

/* ─── Client launch ───
 * Launch a client on this adapter's socket. `command` is a
 * whitespace-separated string (e.g. "weston-terminal").
 * Returns the child PID on success, -1 on fork/exec failure. */
int32_t mansion_adapter_launch_client(void *handle, const char *command);

/* ─── Seat configuration ─── */

/* Set the keyboard modifier state (for GDScript to query).
 * bitfield: bit 0 = shift, bit 1 = control, bit 2 = alt, bit 3 = meta */
void mansion_adapter_set_modifiers(void *handle, uint32_t mods);

/* Get the currently focused surface's client serial (for GDScript queries). */
uint32_t mansion_adapter_get_focused_serial(void *handle);

/* ─── Shutdown ─── */

/* Request clean shutdown. After this returns the compositor core
 * should be torn down (call mansion_adapter_destroy). */
void mansion_adapter_shutdown(void *handle);

#ifdef __cplusplus
}
#endif
