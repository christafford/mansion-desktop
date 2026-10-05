/*
 * MansionGDExtensionAdapter - C implementation of GDExtension adapter
 *
 * This file wraps the compositor-core library and provides a minimal
 * C API for the GDExtension to use. It creates a Wayland display and
 * sets up the basic compositor infrastructure.
 */

#include "mansion_gdextension_adapter.h"

#include <cstring>
#include <cstdio>
#include <cstdlib>

#include <wayland-server.h>
#include <wayland-server-core.h>
#include <wayland-client-core.h>

#include "compositor-core.h"
#include "compositor-private.h"
#include "mansion_bridge.h"

/* ─── Exported API ─── */

extern "C" {

MansionGDExtensionAdapter* mansion_gdextension_adapter_create(
    const MansionGDExtensionConfig *config) {
    
    if (!config) {
        fprintf(stderr, "[gdextension] config is NULL\n");
        return nullptr;
    }

    MansionGDExtensionAdapter *adapter = new MansionGDExtensionAdapter();
    if (!adapter) {
        fprintf(stderr, "[gdextension] failed to allocate adapter\n");
        return nullptr;
    }

    memset(adapter, 0, sizeof(MansionGDExtensionAdapter));
    adapter->debug_logging = config->debug_logging;

    /* Use existing display if provided, otherwise create a new one */
    if (config->display) {
        adapter->display = config->display;
        adapter->owned_display = 0;  /* We don't own the display */
        if (adapter->debug_logging) {
            fprintf(stderr, "[gdextension] using existing display (not owned)\n");
        }
    } else {
        /* Create Wayland display */
        adapter->display = wl_display_create();
        if (!adapter->display) {
            fprintf(stderr, "[gdextension] failed to create Wayland display\n");
            delete adapter;
            return nullptr;
        }
        adapter->owned_display = 1;  /* We own this display */
        if (adapter->debug_logging) {
            fprintf(stderr, "[gdextension] display created (owned)\n");
        }

        /* Set socket name if provided */
        if (config->socket_name && strlen(config->socket_name) > 0) {
            int result = wl_display_add_socket(adapter->display, config->socket_name);
            if (result != 0) {
                fprintf(stderr, "[gdextension] failed to add socket '%s': %s\n",
                        config->socket_name, strerror(-result));
                wl_display_destroy(adapter->display);
                delete adapter;
                return nullptr;
            }
            if (adapter->debug_logging) {
                fprintf(stderr, "[gdextension] socket '%s' added\n", config->socket_name);
            }
        }
    }

    /* Get event loop */
    adapter->event_loop = wl_display_get_event_loop(adapter->display);
    if (!adapter->event_loop) {
        fprintf(stderr, "[gdextension] failed to get event loop\n");
        if (!config->display) {
            /* Only destroy if we created the display */
            wl_display_destroy(adapter->display);
        }
        delete adapter;
        return nullptr;
    }

    /* Use existing compositor if provided, otherwise create a new one */
    if (config->compositor) {
        adapter->compositor = config->compositor;
        adapter->owned_compositor = 0;  /* We don't own the compositor */
        if (adapter->debug_logging) {
            fprintf(stderr, "[gdextension] using existing compositor (not owned)\n");
        }
    } else {
        /* Create compositor */
        adapter->compositor = compositor_core_create(adapter->display);
        if (!adapter->compositor) {
            fprintf(stderr, "[gdextension] failed to create compositor\n");
            if (adapter->owned_display) {
                /* Only destroy if we created the display */
                wl_display_destroy(adapter->display);
            }
            delete adapter;
            return nullptr;
        }
        adapter->owned_compositor = 1;  /* We own this compositor */

        if (adapter->debug_logging) {
            fprintf(stderr, "[gdextension] adapter created successfully\n");
        }
    }

    return adapter;
}

void mansion_gdextension_adapter_destroy(MansionGDExtensionAdapter *adapter) {
    if (!adapter) return;

    if (adapter->debug_logging) {
        fprintf(stderr, "[gdextension] destroying adapter\n");
    }

    /* Destroy compositor only if we own it */
    if (adapter->compositor && adapter->owned_compositor) {
        compositor_core_destroy(adapter->compositor);
        adapter->compositor = nullptr;
    }

    /* Destroy display only if we own it */
    if (adapter->display && adapter->owned_display) {
        wl_display_destroy(adapter->display);
        adapter->display = nullptr;
    }

    delete adapter;
}

struct wl_display* mansion_gdextension_adapter_get_display(
    MansionGDExtensionAdapter *adapter) {
    return adapter ? adapter->display : nullptr;
}

struct MansionCompositor* mansion_gdextension_adapter_get_compositor(
    MansionGDExtensionAdapter *adapter) {
    return adapter ? adapter->compositor : nullptr;
}

int mansion_gdextension_adapter_pump(MansionGDExtensionAdapter *adapter) {
    if (!adapter || !adapter->display) return -1;

    /* Only dispatch events if we own the display.
     * When using an existing display (from main.cpp), the main loop
     * handles event dispatching. We only need to flush events to clients. */
    if (adapter->owned_display) {
        int result = wl_display_dispatch_pending(adapter->display);
        if (result < 0) {
            fprintf(stderr, "[gdextension] wl_display_dispatch_pending failed: %s\n",
                    strerror(-result));
            return -1;
        }
        return result;
    }

    /* When using existing display, just flush to clients (no dispatch) */
    wl_display_flush_clients(adapter->display);
    return 0;
}

int mansion_gdextension_adapter_flush(MansionGDExtensionAdapter *adapter) {
    if (!adapter || !adapter->display) return -1;

    /* wl_display_flush_clients returns void */
    wl_display_flush_clients(adapter->display);
    return 0;
}

struct wl_resource* mansion_gdextension_adapter_create_surface(
    MansionGDExtensionAdapter *adapter,
    struct wl_client *client,
    uint32_t id) {
    
    if (!adapter || !adapter->compositor || !client) {
        return nullptr;
    }

    return compositor_core_create_surface(adapter->compositor, client, id);
}

void mansion_gdextension_adapter_destroy_surface(
    MansionGDExtensionAdapter *adapter,
    struct wl_resource *surface) {
    
    if (!adapter || !surface) return;
    compositor_core_destroy_surface(surface);
}

void mansion_gdextension_adapter_set_focus(
    MansionGDExtensionAdapter *adapter,
    struct wl_resource *surface,
    uint32_t serial) {
    
    if (!adapter || !adapter->compositor) return;
    compositor_core_set_focus(adapter->compositor, surface, serial);
}

void mansion_gdextension_adapter_clear_focus(MansionGDExtensionAdapter *adapter) {
    if (!adapter || !adapter->compositor) return;
    compositor_core_clear_focus(adapter->compositor);
}

struct wl_resource* mansion_gdextension_adapter_get_focused_surface(
    MansionGDExtensionAdapter *adapter) {
    if (!adapter || !adapter->compositor) return nullptr;
    return compositor_core_get_focused_surface(adapter->compositor);
}

uint32_t mansion_gdextension_adapter_get_focused_serial(
    MansionGDExtensionAdapter *adapter) {
    if (!adapter || !adapter->compositor) return 0;
    return compositor_core_get_focus_serial(adapter->compositor);
}

/* ─── Shm frame snapshot API ─── */

MansionShmSnapshot* mansion_gdextension_adapter_create_snapshot(
    MansionGDExtensionAdapter *adapter,
    uint32_t client_serial) {
    if (!adapter) return nullptr;

    /* Find the surface by client serial */
    struct MansionSurface *surface = compositor_surface_from_serial(adapter->compositor, client_serial);
    if (!surface) {
        return nullptr;
    }

    /* Check if there's a committed buffer */
    if (!surface->buffer_resource || surface->buffer_destroyed) {
        return nullptr;
    }

    struct wl_shm_buffer *shm_buf = wl_shm_buffer_get(surface->buffer_resource);
    if (!shm_buf) {
        return nullptr;
    }

    /* Get buffer properties */
    int32_t width = wl_shm_buffer_get_width(shm_buf);
    int32_t height = wl_shm_buffer_get_height(shm_buf);
    int32_t stride = wl_shm_buffer_get_stride(shm_buf);
    int32_t format = wl_shm_buffer_get_format(shm_buf);

    /* Validate stride */
    int32_t min_stride = width * 4;  /* ARGB8888 = 4 bytes/pixel */
    if (stride < min_stride || stride % 4 != 0) {
        return nullptr;
    }

    /* Allocate snapshot */
    MansionShmSnapshot *snapshot = new MansionShmSnapshot;
    snapshot->width = width;
    snapshot->height = height;
    snapshot->stride = stride;
    snapshot->format = format;
    snapshot->revision = 1;  /* First snapshot */
    snapshot->pixels_size = stride * height;
    snapshot->pixels = (uint8_t *)malloc(snapshot->pixels_size);

    if (!snapshot->pixels) {
        delete snapshot;
        return nullptr;
    }

    /* Access and copy pixels */
    wl_shm_buffer_begin_access(shm_buf);
    void *data = wl_shm_buffer_get_data(shm_buf);

    if (data) {
        /* Convert to ARGB8888 format (0xAARRGGBB, little-endian) */
        for (int32_t y = 0; y < height; ++y) {
            uint8_t *src_row = (uint8_t *)data + y * stride;
            uint8_t *dst_row = snapshot->pixels + y * width * 4;

            for (int32_t x = 0; x < width; ++x) {
                uint32_t pixel;

                if (format == WL_SHM_FORMAT_ABGR8888 || format == WL_SHM_FORMAT_XBGR8888) {
                    /* ABGR8888 on little-endian: bytes are [R, G, B, A] */
                    uint8_t r = src_row[x * 4 + 0];
                    uint8_t g = src_row[x * 4 + 1];
                    uint8_t b = src_row[x * 4 + 2];
                    uint8_t a = (format == WL_SHM_FORMAT_ABGR8888) ? src_row[x * 4 + 3] : 0xFF;
                    pixel = (a << 24) | (r << 16) | (g << 8) | b;  /* ARGB8888 */
                } else {
                    /* Default: ARGB8888 / XRGB8888 on little-endian have bytes [B, G, R, A] */
                    uint8_t b = src_row[x * 4 + 0];
                    uint8_t g = src_row[x * 4 + 1];
                    uint8_t r = src_row[x * 4 + 2];
                    uint8_t a = (format == WL_SHM_FORMAT_ARGB8888) ? src_row[x * 4 + 3] : 0xFF;
                    pixel = (a << 24) | (r << 16) | (g << 8) | b;  /* ARGB8888 */
                }

                /* Store as 32-bit ARGB8888 (little-endian: 0xAARRGGBB) */
                ((uint32_t *)dst_row)[x] = pixel;
            }
        }
    }

    wl_shm_buffer_end_access(shm_buf);

    return snapshot;
}

void mansion_gdextension_adapter_destroy_snapshot(MansionShmSnapshot *snapshot) {
    if (!snapshot) return;
    free(snapshot->pixels);
    delete snapshot;
}

int32_t mansion_gdextension_snapshot_get_width(MansionShmSnapshot *snapshot) {
    return snapshot ? snapshot->width : 0;
}

int32_t mansion_gdextension_snapshot_get_height(MansionShmSnapshot *snapshot) {
    return snapshot ? snapshot->height : 0;
}

int32_t mansion_gdextension_snapshot_get_stride(MansionShmSnapshot *snapshot) {
    return snapshot ? snapshot->stride : 0;
}

int32_t mansion_gdextension_snapshot_get_format(MansionShmSnapshot *snapshot) {
    return snapshot ? snapshot->format : 0;
}

uint32_t mansion_gdextension_snapshot_get_revision(MansionShmSnapshot *snapshot) {
    return snapshot ? snapshot->revision : 0;
}

uint8_t* mansion_gdextension_snapshot_get_pixels(MansionShmSnapshot *snapshot) {
    return snapshot ? snapshot->pixels : nullptr;
}

size_t mansion_gdextension_snapshot_get_pixels_size(MansionShmSnapshot *snapshot) {
    return snapshot ? snapshot->pixels_size : 0;
}

int mansion_gdextension_snapshot_copy_pixels(MansionShmSnapshot *snapshot, uint8_t *out_buffer, size_t buffer_size) {
    if (!snapshot || !out_buffer) {
        return -1;
    }
    
    if (buffer_size < snapshot->pixels_size) {
        return -1;  /* Buffer too small */
    }
    
    memcpy(out_buffer, snapshot->pixels, snapshot->pixels_size);
    return (int)snapshot->pixels_size;
}

} // extern "C"
