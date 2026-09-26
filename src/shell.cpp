#include <string>

#include <wayland-server-protocol.h>
#include <wayland-util.h>

#include "shell.h"
#include "compositor.h"

struct MansionShellSurface {
    struct MansionShell* shell;
    struct wl_resource* resource;
    struct wl_resource* surface;
    struct wl_list link;

    struct {
        bool is_fullscreen;
        uint32_t method;
        uint32_t framerate;
        int32_t width, height;
    } pending;

    struct {
        bool is_fullscreen;
        uint32_t method;
        uint32_t framerate;
        int32_t width, height;
    } current;

    std::string title;
    std::string class_name;
    bool is_modal;
    bool is_top_hint;
    bool is_maximized;
    bool is_minimized;
};

struct MansionShell {
    struct wl_global* global;
    struct wl_list shell_surface_list;
};

static void shell_surface_pong(struct wl_client* client, struct wl_resource* resource,
                                uint32_t serial) {
    (void)client; (void)resource; (void)serial;
}

static void shell_surface_move(struct wl_client* client, struct wl_resource* resource,
                                struct wl_resource* seat, uint32_t serial) {
    (void)client; (void)resource; (void)seat; (void)serial;
}

static void shell_surface_resize(struct wl_client* client, struct wl_resource* resource,
                                  struct wl_resource* seat, uint32_t serial, uint32_t edges) {
    (void)client; (void)resource; (void)seat; (void)serial; (void)edges;
}

static void shell_surface_set_transient(struct wl_client* client, struct wl_resource* resource,
                                         struct wl_resource* parent,
                                         int32_t x, int32_t y, uint32_t flags) {
    (void)client; (void)resource; (void)parent; (void)x; (void)y; (void)flags;
}

static void shell_surface_set_fullscreen(struct wl_client* client, struct wl_resource* resource,
                                           uint32_t method, uint32_t framerate,
                                           struct wl_resource* output) {
    (void)client; (void)output;
    auto* shell_surface = static_cast<MansionShellSurface*>(wl_resource_get_user_data(resource));
    shell_surface->pending.is_fullscreen = true;
    shell_surface->pending.method = method;
    shell_surface->pending.framerate = framerate;
}

static void shell_surface_set_popup(struct wl_client* client, struct wl_resource* resource,
                                      struct wl_resource* seat, uint32_t serial, struct wl_resource* parent,
                                      int32_t x, int32_t y, uint32_t flags) {
    (void)client; (void)resource; (void)seat; (void)serial; (void)parent;
    (void)x; (void)y; (void)flags;
}

static void shell_surface_set_maximized(struct wl_client* client, struct wl_resource* resource,
                                         struct wl_resource* output) {
    (void)client; (void)resource; (void)output;
    auto* shell_surface = static_cast<MansionShellSurface*>(wl_resource_get_user_data(resource));
    shell_surface->is_maximized = true;
}

static void shell_surface_set_toplevel(struct wl_client* client, struct wl_resource* resource) {
    (void)client; (void)resource;
    auto* shell_surface = static_cast<MansionShellSurface*>(wl_resource_get_user_data(resource));
    shell_surface->pending.is_fullscreen = false;
    shell_surface->is_maximized = false;
    shell_surface->is_minimized = false;
}

static void shell_surface_set_title(struct wl_client* client, struct wl_resource* resource,
                                     const char* title) {
    (void)client;
    auto* shell_surface = static_cast<MansionShellSurface*>(wl_resource_get_user_data(resource));
    shell_surface->title = title ? title : "";
}

static void shell_surface_set_class(struct wl_client* client, struct wl_resource* resource,
                                     const char* class_) {
    (void)client;
    auto* shell_surface = static_cast<MansionShellSurface*>(wl_resource_get_user_data(resource));
    shell_surface->class_name = class_ ? class_ : "";
}

static void shell_surface_destroy(struct wl_resource* resource) {
    auto* shell_surface = static_cast<MansionShellSurface*>(wl_resource_get_user_data(resource));
    wl_list_remove(&shell_surface->link);
    delete shell_surface;
}

static const struct wl_shell_surface_interface shell_surface_impl = {
    shell_surface_pong,
    shell_surface_move,
    shell_surface_resize,
    shell_surface_set_toplevel,
    shell_surface_set_transient,
    shell_surface_set_fullscreen,
    shell_surface_set_popup,
    shell_surface_set_maximized,
    shell_surface_set_title,
    shell_surface_set_class,
};

static void shell_create_shell_surface(struct wl_client* client, struct wl_resource* shell_resource,
                                        uint32_t id, struct wl_resource* surface) {
    auto* shell = static_cast<MansionShell*>(wl_resource_get_user_data(shell_resource));

    auto* shell_surface = new MansionShellSurface;
    shell_surface->shell = shell;
    shell_surface->resource = wl_resource_create(client, &wl_shell_surface_interface, 1, id);
    if (!shell_surface->resource) {
        delete shell_surface;
        wl_client_post_no_memory(client);
        return;
    }

    shell_surface->surface = surface;
    wl_resource_set_implementation(shell_surface->resource, &shell_surface_impl,
                                    shell_surface, shell_surface_destroy);

    wl_list_init(&shell_surface->link);
    wl_list_insert(&shell->shell_surface_list, &shell_surface->link);

    shell_surface->pending.is_fullscreen = false;
    shell_surface->current.is_fullscreen = false;
    shell_surface->is_modal = false;
    shell_surface->is_top_hint = false;
    shell_surface->is_maximized = false;
    shell_surface->is_minimized = false;
}

static const struct wl_shell_interface shell_impl = {
    shell_create_shell_surface,
};

static void shell_bind(struct wl_client* client, void* data, uint32_t version, uint32_t id) {
    auto* shell = static_cast<MansionShell*>(data);
    (void)version;

    auto* resource = wl_resource_create(client, &wl_shell_interface, 1, id);
    if (!resource) {
        wl_client_post_no_memory(client);
        return;
    }
    wl_resource_set_implementation(resource, &shell_impl, shell, nullptr);
}

struct MansionShell* create_shell(struct MansionCompositor* compositor, struct wl_display* display) {
    (void)compositor;
    auto* shell = new MansionShell;
    wl_list_init(&shell->shell_surface_list);

    shell->global = wl_global_create(display, &wl_shell_interface, 1, shell, shell_bind);
    if (!shell->global) {
        delete shell;
        return nullptr;
    }

    return shell;
}

void destroy_shell(struct MansionShell* shell) {
    if (!shell) return;
    wl_global_destroy(shell->global);

    struct MansionShellSurface *surface, *next;
    wl_list_for_each_safe(surface, next, &shell->shell_surface_list, link) {
        wl_resource_destroy(surface->resource);
    }

    delete shell;
}
