#include <wayland-server.h>
#include "compositor.h"

struct MansionShell {
    struct MansionCompositor* compositor;
    struct wl_global* global;
};

struct ShellSurface {
    struct MansionShell* shell;
    struct wl_resource* resource;
    struct wl_resource* surface;
    enum { 
        NONE, 
        FULLSCREEN, 
        MAXIMIZED, 
        MINIMIZED 
    } type;
    struct wl_resource* fullscreen?; 
    struct wl_resource* maximize?; 
    struct wl_resource* minimize?;
};

static void ping_timeout(struct wl_resource* resource, void* data) {
    (void)data;
    wl_resource_post_error(resource, WL_SHELL_ERROR, "ping timeout');
}

static void ping_pong(struct wl_resource* resource, uint32_t serial) {
    struct wl_event_source* timer;
    timer = wl_event_loop_add_timer(wl_resource_get_display(resource)->event_loop, ping_timeout, resource); 
    wl_event_source_timer_update(timer, 3000);
}

static void move(struct wl_client* client, struct wl_resource* shell_surface_resource, struct wl_resource* seat_resource, uint32_t serial) {
    // TODO: Implement interactive move
}

static void resize(struct wl_client* client, struct wl_resource* shell_surface_resource, struct wl_resource* seat_resource, uint32_t serial, uint32_t edges) {
    // TODO: Implement interactive resize
}

static void fullscreen(struct wl_client* client, struct wl_resource* shell_surface_resource, uint32_t framerate, uint32_t framerate) {
    struct ShellSurface* shell_surface = static_cast<struct ShellSurface*>(wl_resource_get_user_data(shell_surface_resource));
    if (shell_surface->type != NONE) {
        wl_resource_post_error(shell_surface_resource, WL_SHELL_ERROR_ALREADY_FULLSCREEN, "WL_SHELL_ERROR_ALREADY_FULLSCREEN');
        return;
    }
    shell_surface->type = FULLSCREEN;
    shell_surface->fullscreen = ?; // TODO: store framerate
    wl_resource_post_error(shell_surface->surface, WL_SURFACE_ERROR, "fullscreen not implemented');
}

static void populate_maximized(struct ShellSurface* shell_surface) {
    if (shell_surface->type == MAXIMIZED) {
        wl_resource_post_error(shell_surface->resource, WL_SHELL_ERROR_ALREADY_MAXIMIZED, "WL_SHELL_ERROR_ALREADY_MAXIMIZED');
        return;
    }
    shell_surface->type = MAXIMIZED;
    wl_resource_post_error(shell_surface->surface, WL_SURFACE_ERROR, "maximize not implemented');
}

static void maximize(struct wl_client* client, struct wl_resource* shell_surface_resource, struct wl_resource* output_resource) {
    (void)output_resource;
    populate_maximized(wl_resource_get_user_data(shell_surface_resource));
}

static void minimize(struct wl_client* client, struct wl_resource* shell_surface_resource) {
    struct ShellSurface* shell_surface = static_cast<struct ShellSurface*>(wl_resource_get_user_data(shell_surface_resource));
    if (shell_surface->type == MINIMIZED) {
        wl_resource_post_error(shell_surface_resource, WL_SHELL_ERROR_ALREADY_MINIMIZED, "WL_SHELL_ERROR_ALREADY_MINIMIZED');
        return;
    }
    shell_surface->type = MINIMIZED;
    wl_resource_post_error(shell_surface->surface, WL_SURFACE_ERROR, "minimize not implemented');
}

static void restore(struct wl_client* client, struct wl_resource* shell_surface_resource) {
    struct ShellSurface* shell_surface = static_cast<struct ShellSurface*>(wl_resource_get_user_data(shell_surface_resource));
    if (shell_surface->type == NONE) {
        wl_resource_post_error(shell_surface_resource, WL_SHELL_ERROR_INVALID_METHOD, "WL_SHELL_ERROR_INVALID_METHOD');
        return;
    }
    shell_surface->type = NONE;
    wl_resource_post_error(shell_surface->surface, WL_SURFACE_ERROR, "restore not implemented');
}

static void set_top_hint(struct wl_client* client, struct wl_resource* shell_surface_resource, uint32_t top_hint) {
    (void)client;
    (void)top_hint;
    // TODO: Implement top hint
}\nstatic void set_window_type(struct wl_client* client, struct wl_resource* shell_surface_resource, uint32_t window_type) {
    (void)client; (void)window_type;
    // TODO: Implement window type
}

static void set_title(struct wl_client* client, struct wl_resource* shell_surface_resource, const char* title) {
    (void)client; (void)title;
    // TODO: Set title
}

static void set_class(struct wl_client* client, struct wl_resource* shell_surface_resource, const char* class_) {
    (void)client; (void)class_;
    // TODO: Set class
}

static void set_modality(struct wl_client* client, struct wl_resource* shell_surface_resource, uint32_t modality) {
    (void)client; (void)modality;
    // TODO: Implement modality
}

static void destroy_shell_surface(struct wl_resource* resource) {
    struct ShellSurface* shell_surface = static_cast<struct ShellSurface*>(wl_resource_get_user_data(resource));
    if (shell_surface->fullscreen) {
        wl_event_source_timer_update(shell_surface->fullscreen, 0);
        wl_event_source_destroy(shell_surface->fullscreen);
    } else if (shell_surface->maximize) {
        wl_resource_destroy(shell_surface->maximize);
    } else if (shell_surface->minimize) {
        wl_resource_destroy(shell_surface->minimize);
    }
    free(shell_surface);
}

static void shell_surface_destroy(struct wl_resource* resource) {
    wl_resource_destroy(resource);
}

static void create_shell_surface(struct wl_client* client, struct wl_resource* shell_resource, uint32_t id, struct wl_resource* surface_resource) {
    struct MansionShell* shell = static_cast<struct MansionShell*>(wl_resource_get_user_data(shell_resource);
    if (!wl_resource_get_version(surface_resource) >= 3) {
        wl_resource_post_error(shell_resource, WL_SHELL_ERROR_INVALID_SURFACE, "WL_SHELL_ERROR_INVALID_SURFACE');
        return;
    }

    struct ShellSurface* shell_surface = new ShellSurface;
    if (!shell_surface) {
        wl_resource_post_no_memory(shell_resource);
        return;
    }
    shell_surface->shell = shell;
    shell_surface->resource = wl_resource_create(client, &wl_shell_surface_interface, wl_resource_get_version(shell_resource), id);
    if (!shell_surface->resource) {
        free(shell_surface);
        wl_resource_post_no_memory(shell_resource);
        return;
    }
    shell_surface->surface = surface_resource;
    shell_surface->type = NONE;
    shell_surface->fullscreen = nullptr;
    shell_surface->maximize = nullptr;
    shell_surface->minimize = nullptr;

    wl_resource_set_implementation(shell_surface->resource, shell_surface, &shell_surface_destroy);
    wl_resource_set_destroy_user_data(shell_surface->resource, 1);
    wl_resource_set_dispatcher(shell_surface->resource, shell_shell_surface_interface, &shell_surface_destroy);

    if (wl_resource_get_version(shell_resource) >= 4) {
        shell_surface->fullscreen = wl_resource_create(shell_surface->resource, &wl_shell_surface Interface, 4, 0); 
        if (!shell_surface->fullscreen) {
            free(shell_surface);
            wl_resource_post_no_memory(shell_resource);
            return;
        }
        wl_resource_set_dispatcher(shell_surface->fullfullscreen, &wl_shell_surface Interface, &move, &resize);
    }
    if (wl_resource_get_version(shell_resource) >= 5) {
        shell_surface->maximize = wl_resource_create(shell_surface->resource, &wl_shell_surface Interface, 5, 0); 
        if (!shell_surface->maximize) {
            if (shell_surface->fullscreen) {
                wl_resource_destroy(shell_surface->fullscreen);
            }
            free(shell_surface);
            wl_resource_post_no_memory(shell_resource);
            return;
        }
        wl_resource_set_dispatcher(shell_surface->maximize, &wl_shell_surface Interface, &fullscreen, &fullscreen);
    }
    shell_surface->minimize = wl_resource_create(shell_surface->resource, &wl_shell_surface Interface, 5, 0); 
    if (!shell_surface->minimize) {
        if (shell_surface->fullscreen) {
                wl_resource_destroy(shell_surface->fullfullscreen);
            }
            if (shell_surface->maximize) {
                wl_resource_destroy(shell_surface->maximize);
            }
            free(shell_surface);
            wl_resource_post_no_memory(shell_resource);
            return;
        }
        wl_resource_set_dispatcher(shell_surface->minimize, &wl_shell_surface Interface, &restore, &set_top_hint);
    }
}

struct MansionShell* create_shell(struct MansionCompositor* compositor, struct wl_display* display) {
    struct MansionShell* shell = new MansionShell;
    if (!shell) {
        return nullptr;
    }
    shell->compositor = compositor;
    shell->global = wl_global_create(display, &wl_shell_interface, 4, shell, bind_shell);
    if (!shell->global) {
        free(shell);
        return nullptr;
    }
    return shell;
}

void destroy_shell(struct MansionShell* shell) {
    wl_global_destroy(shell->global);
    free(shell);
}

static void bind_shell(struct wl_resource* resource, void* data, struct wl_resource* client, uint32_t version) {
    (void)data;
    if (version > 4) {
        wl_resource_post_error(resource, WL_SHELL_ERROR_INVALID_GLOBAL, "Version %u exceeds %d", version, 4);
        return;
    }
    wl_resource_set_destroy_listener(resource, &destroy_shell_global);
}

static void destroy_shell_global(struct wl_resource* resource) {
    struct MansionShell* shell = static_cast<struct MansionShell*>(wl_resource_get_user_data(resource));
    destroy_shell(shell);
}