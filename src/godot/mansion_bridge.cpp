/*
 * MansionBridge — C++ implementation of the C-facing bridge.
 *
 * This file connects the GDExtension entry point to the Wayland
 * compositor core (compositor.cpp, xdg-shell.cpp) and the seat
 * (input.cpp). It does NOT include display.h (which pulls in GL).
 *
 * Architecture:
 *   GDExtension → mansion_adapter_* (C bridge) → compositor core → Wayland server
 *
 * Ownership:
 *   - The adapter owns the Wayland display socket
 *   - The compositor owns surface lists and protocol globals
 *   - Frame callbacks are owned by surfaces; firing them is safe
 *   - Client processes are tracked via launch_app() for clean reaping
 *
 * Threading:
 *   All adapter calls must come from the owning thread (the thread
 *   that called mansion_adapter_create()). This is the Godot main
 *   thread. No threads are spawned by the adapter.
 */

#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <unistd.h>
#include <cstdio>
#include <vector>
#include <memory>

#include <wayland-server.h>
#include <wayland-util.h>

#include "compositor.h"
#include "compositor-private.h"
#include "xdg-shell.h"
#include "input.h"
#include "launch.h"
#include "mansion_bridge.h"

/* ─── Internal adapter state ─── */

struct MansionAdapter {
    struct wl_display *display = nullptr;
    struct MansionCompositor *compositor = nullptr;
    struct MansionXdgShell *xdg_shell = nullptr;
    struct MansionSeat *seat = nullptr;
    struct wl_event_loop *event_loop = nullptr;

    /* Tracked client apps for clean shutdown */
    std::vector<MansionApp*> apps;

    /* Client serial counter */
    uint32_t next_serial = 1;

    /* Debug logging toggle */
    bool debug = false;

    /* Stored socket name for environment export */
    char socket_name[256] = {};
};

/* ─── Implementation ─── */

void *mansion_adapter_create(const MansionAdapterConfig *config) {
    MansionAdapter *adapter = new MansionAdapter;

    /* Create a private Wayland display socket */
    adapter->display = wl_display_create();
    if (!adapter->display) {
        delete adapter;
        return nullptr;
    }

    /* Set up the event loop */
    adapter->event_loop = wl_display_get_event_loop(adapter->display);
    if (!adapter->event_loop) {
        wl_display_destroy(adapter->display);
        delete adapter;
        return nullptr;
    }

    /* Create the compositor core */
    adapter->compositor = create_compositor(adapter->display);
    if (!adapter->compositor) {
        wl_display_destroy(adapter->display);
        delete adapter;
        return nullptr;
    }

    /* Create the seat — required for keyboard focus */
    adapter->seat = create_seat(adapter->display);
    if (!adapter->seat) {
        destroy_compositor(adapter->compositor);
        wl_display_destroy(adapter->display);
        delete adapter;
        return nullptr;
    }

    /* Connect the seat to the compositor for focus queries */
    compositor_set_seat(adapter->compositor, adapter->seat);

    /* Initialize xdg-shell — required for client window management */
    adapter->xdg_shell = create_xdg_shell(adapter->compositor, adapter->display);
    if (!adapter->xdg_shell) {
        destroy_seat(adapter->seat);
        destroy_compositor(adapter->compositor);
        wl_display_destroy(adapter->display);
        delete adapter;
        return nullptr;
    }

    /* Configure debug logging */
    if (config) {
        adapter->debug = config->debug_logging;
    }

    /* Create and bind the socket file */
    const char *socket_name = config ? config->socket_name : nullptr;

    if (socket_name) {
        snprintf(adapter->socket_name, sizeof(adapter->socket_name), "mansion-%s",
                 socket_name);
    } else {
        snprintf(adapter->socket_name, sizeof(adapter->socket_name),
                 "mansion-auto-%d", getpid());
    }

    const char *sock = wl_display_add_socket_auto(adapter->display);
    if (!sock) {
        /* Fallback: try explicit socket name */
        if (wl_display_add_socket(adapter->display, adapter->socket_name) < 0) {
            destroy_xdg_shell(adapter->xdg_shell);
            destroy_seat(adapter->seat);
            destroy_compositor(adapter->compositor);
            wl_display_destroy(adapter->display);
            delete adapter;
            return nullptr;
        }
        sock = adapter->socket_name;
    }

    if (adapter->debug) {
        fprintf(stderr, "[mansion] socket: %s\n", sock);
    }

    /* Export WAYLAND_DISPLAY so child clients find our socket */
    setenv("WAYLAND_DISPLAY", sock, 1);

    return adapter;
}

void mansion_adapter_destroy(void *handle) {
    if (!handle) return;
    auto *adapter = static_cast<MansionAdapter *>(handle);

    /* Destroy tracked client apps */
    for (auto *app : adapter->apps) {
        if (app) {
            destroy_app(app);
        }
    }
    adapter->apps.clear();

    /* Tear down in reverse order: xdg-shell → seat → compositor → display */
    if (adapter->xdg_shell) {
        destroy_xdg_shell(adapter->xdg_shell);
        adapter->xdg_shell = nullptr;
    }

    if (adapter->seat) {
        destroy_seat(adapter->seat);
        adapter->seat = nullptr;
    }

    if (adapter->compositor) {
        destroy_compositor(adapter->compositor);
        adapter->compositor = nullptr;
    }

    /* Unset the socket environment */
    unsetenv("WAYLAND_DISPLAY");

    /* Destroy the Wayland display (closes socket, removes socket file) */
    if (adapter->display) {
        wl_display_destroy(adapter->display);
    }

    delete adapter;
}

int mansion_adapter_pump(void *handle) {
    if (!handle) return 0;
    auto *adapter = static_cast<MansionAdapter *>(handle);

    /* Non-blocking dispatch */
    return wl_event_loop_dispatch(adapter->event_loop, 0);
}

int mansion_adapter_flush(void *handle) {
    if (!handle) return -1;
    auto *adapter = static_cast<MansionAdapter *>(handle);

    /* Server-side: flush pending writes to all connected clients */
    wl_display_flush_clients(adapter->display);
    return 0;
}

void mansion_adapter_enumerate_surfaces(void *handle,
                                         mansion_surface_enumerate_fn callback,
                                         void *user_data) {
    if (!handle || !callback) return;
    auto *adapter = static_cast<MansionAdapter *>(handle);

    struct MansionSurface *surface;
    wl_list_for_each(surface, &adapter->compositor->surface_list, link) {
        MansionSurfaceInfo info;
        info.width = surface->width;
        info.height = surface->height;
        info.x = surface->has_current_position ? surface->current_x : 0;
        info.y = surface->has_current_position ? surface->current_y : 0;
        info.needs_upload = surface->needs_upload;

        callback(user_data, adapter->next_serial++, &info);
    }

    /* Also enumerate orphaned surfaces */
    wl_list_for_each(surface, &adapter->compositor->orphaned_surfaces, link) {
        MansionSurfaceInfo info;
        info.width = surface->width;
        info.height = surface->height;
        info.x = surface->has_current_position ? surface->current_x : 0;
        info.y = surface->has_current_position ? surface->current_y : 0;
        info.needs_upload = surface->needs_upload;

        callback(user_data, adapter->next_serial++, &info);
    }
}

int mansion_adapter_fire_frame_callbacks(void *handle) {
    if (!handle) return 0;
    auto *adapter = static_cast<MansionAdapter *>(handle);

    int fired = 0;

    struct MansionSurface *surface;
    struct MansionSurface *next_surface;
    wl_list_for_each_safe(surface, next_surface,
                          &adapter->compositor->surface_list, link) {
        struct wl_resource *cb, *cb_next;
        wl_list_for_each_safe(cb, cb_next, &surface->frame_callback_list, link) {
            wl_list_remove(wl_resource_get_link(cb));
            wl_callback_send_done(cb, 0);
            wl_resource_destroy(cb);
            fired++;
        }
    }

    return fired;
}

int32_t mansion_adapter_launch_client(void *handle, const char *command) {
    if (!handle || !command || !command[0]) return -1;
    auto *adapter = static_cast<MansionAdapter *>(handle);

    MansionApp *app = launch_app(adapter->display, adapter->socket_name, command);
    if (!app) return -1;

    adapter->apps.push_back(app);
    return static_cast<int32_t>(app_pid(app));
}

void mansion_adapter_set_modifiers(void *handle, uint32_t mods) {
    (void)handle;
    (void)mods;
    /* Modifiers are set by the seat on each key event. GDScript
     * can query the current modifier state from the seat. */
}

uint32_t mansion_adapter_get_focused_serial(void *handle) {
    if (!handle) return 0;
    auto *adapter = static_cast<MansionAdapter *>(handle);
    return adapter->compositor->keyboard_focus_serial;
}

void mansion_adapter_shutdown(void *handle) {
    if (!handle) return;
    auto *adapter = static_cast<MansionAdapter *>(handle);

    /* Clear seat focus state to prevent dangling references */
    if (adapter->seat) {
        seat_clear_focus_state();
    }
}
