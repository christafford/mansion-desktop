#define _GNU_SOURCE
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include <xkbcommon/xkbcommon.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

static void require(int condition, const char *message) {
    if (!condition) { fprintf(stderr, "CLIENT FAIL: %s\n", message); exit(1); }
}
struct seat_state { int caps, names, keymaps, repeats; uint32_t version; };
struct globals {
    struct wl_compositor *compositor;
    struct wl_shm *shm;
    struct xdg_wm_base *shell;
    struct wl_seat *seats[2];
    struct seat_state seat_state[2];
};
static void capabilities(void *data, struct wl_seat *seat, uint32_t caps) {
    (void)seat;
    require(caps == (WL_SEAT_CAPABILITY_POINTER | WL_SEAT_CAPABILITY_KEYBOARD), "seat capabilities");
    ((struct seat_state*)data)->caps++;
}
static void seat_name(void *data, struct wl_seat *seat, const char *name) {
    (void)seat;
    require(name && *name, "seat name");
    ((struct seat_state*)data)->names++;
}
static const struct wl_seat_listener seat_listener = { capabilities, seat_name };
static void keymap(void *data, struct wl_keyboard *keyboard, uint32_t format, int fd, uint32_t size) {
    (void)keyboard;
    require(format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1 && size > 1, "keymap format");
    char *text = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    require(text != MAP_FAILED && text[size - 1] == '\0', "keymap fd must include NUL");
    struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    require(context != NULL, "xkb context");
    struct xkb_keymap *map = xkb_keymap_new_from_string(context, text, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
    require(map != NULL, "keymap parses");
    xkb_keymap_unref(map);
    xkb_context_unref(context);
    munmap(text, size);
    close(fd);
    ((struct seat_state*)data)->keymaps++;
}
static void repeat_info(void *data, struct wl_keyboard *keyboard, int32_t rate, int32_t delay) {
    (void)keyboard;
    require(rate == 25 && delay == 500, "repeat settings");
    ((struct seat_state*)data)->repeats++;
}
// No focus/input events are expected in this protocol-only trial.
static const struct wl_keyboard_listener keyboard_listener = { .keymap = keymap, .repeat_info = repeat_info };
static void ping(void *data, struct xdg_wm_base *shell, uint32_t serial) {
    (void)data; xdg_wm_base_pong(shell, serial);
}
static const struct xdg_wm_base_listener shell_listener = { ping };
static void global(void *data, struct wl_registry *registry, uint32_t id,
                   const char *interface, uint32_t version) {
    struct globals *g = data;
    if (!strcmp(interface, "wl_compositor"))
        g->compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 1);
    if (!strcmp(interface, "wl_shm"))
        g->shm = wl_registry_bind(registry, id, &wl_shm_interface, 1);
    if (!strcmp(interface, "xdg_wm_base")) {
        require(version == 3, "xdg version");
        g->shell = wl_registry_bind(registry, id, &xdg_wm_base_interface, 3);
        xdg_wm_base_add_listener(g->shell, &shell_listener, NULL);
    }
    if (!strcmp(interface, "wl_seat")) {
        require(version == 4, "seat advertised version");
        for (int i = 0; i < 2; ++i) {
            g->seat_state[i].version = i ? 4 : 1;
            g->seats[i] = wl_registry_bind(registry, id, &wl_seat_interface, g->seat_state[i].version);
            wl_seat_add_listener(g->seats[i], &seat_listener, &g->seat_state[i]);
        }
    }
}
static void removed(void *data, struct wl_registry *registry, uint32_t name) {
    (void)data; (void)registry; (void)name;
}
static const struct wl_registry_listener listener = {global, removed};
struct window {
    struct wl_surface *surface;
    struct xdg_surface *xdg;
    struct xdg_toplevel *top;
    struct wl_callback *frame;
    int role_configures, configures;
    uint32_t serial;
};
static void frame_done(void *data, struct wl_callback *callback, uint32_t time) {
    (void)time;
    ((struct window*)data)->frame = NULL;
    wl_callback_destroy(callback);
}
static const struct wl_callback_listener frame_listener = { frame_done };
static void released(void *data, struct wl_buffer *buffer) { (void)data; (void)buffer; }
static const struct wl_buffer_listener buffer_listener = { released };
static void configured(void *data, struct xdg_surface *surface, uint32_t serial) {
    struct window *w = data;
    require(w->role_configures == w->configures + 1, "role configure before surface configure");
    w->configures++;
    w->serial = serial;
    xdg_surface_ack_configure(surface, serial);
}
static const struct xdg_surface_listener surface_listener = { configured };
static void top_configured(void *data, struct xdg_toplevel *top, int32_t width, int32_t height, struct wl_array *states) {
    (void)top; (void)states;
    require(width == 800 && height == 600, "initial toplevel dimensions");
    ((struct window*)data)->role_configures++;
}
static void closed(void *data, struct xdg_toplevel *top) { (void)data; (void)top; require(0, "unexpected close"); }
static const struct xdg_toplevel_listener top_listener = { .configure = top_configured, .close = closed };
static void create_window(struct globals *g, struct window *w, int with_role) {
    w->surface = wl_compositor_create_surface(g->compositor);
    w->xdg = xdg_wm_base_get_xdg_surface(g->shell, w->surface);
    xdg_surface_add_listener(w->xdg, &surface_listener, w);
    if (with_role) {
        w->top = xdg_surface_get_toplevel(w->xdg);
        xdg_toplevel_add_listener(w->top, &top_listener, w);
        xdg_toplevel_set_title(w->top, "Runtime protocol fixture");
        xdg_toplevel_set_app_id(w->top, "mansion.runtime-test");
    }
}
static void destroy_window(struct window *w, int local) {
    if (w->frame) wl_callback_destroy(w->frame);
    if (local) {
        if (w->top) wl_proxy_destroy((struct wl_proxy*)w->top);
        wl_proxy_destroy((struct wl_proxy*)w->xdg);
        wl_proxy_destroy((struct wl_proxy*)w->surface);
    } else {
        if (w->top) xdg_toplevel_destroy(w->top);
        xdg_surface_destroy(w->xdg);
        wl_surface_destroy(w->surface);
    }
}
static void marker(const char *base, const char *suffix) {
    char path[1024];
    require(snprintf(path, sizeof path, "%s.%s", base, suffix) < (int)sizeof path, "marker path");
    int fd = open(path, O_CREAT | O_WRONLY | O_CLOEXEC, 0600);
    require(fd >= 0, "marker open"); close(fd);
}
static void barrier(const char *base, const char *suffix) {
    char path[1024];
    require(snprintf(path, sizeof path, "%s.%s", base, suffix) < (int)sizeof path, "barrier path");
    while (access(path, F_OK) < 0) usleep(1000);
}
int main(int argc, char **argv) {
    require(argc == 4, "socket, marker and mode arguments");
    alarm(12);
    const char *mode = argv[3];
    struct wl_display *display = wl_display_connect(argv[1]);
    require(display != NULL, "connect");
    struct globals g = {0};
    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &listener, &g);
    require(wl_display_roundtrip(display) >= 0 && g.compositor && g.shm && g.shell && g.seats[0] && g.seats[1], "globals");
    struct wl_pointer *pointers[2];
    struct wl_keyboard *keyboards[2];
    for (int i = 0; i < 2; ++i) {
        pointers[i] = wl_seat_get_pointer(g.seats[i]);
        keyboards[i] = wl_seat_get_keyboard(g.seats[i]);
        wl_keyboard_add_listener(keyboards[i], &keyboard_listener, &g.seat_state[i]);
    }
    require(wl_display_roundtrip(display) >= 0, "seat roundtrip");
    for (int i = 0; i < 2; ++i)
        require(g.seat_state[i].caps == 1 && g.seat_state[i].names == i && g.seat_state[i].keymaps == 1 && g.seat_state[i].repeats == i, "seat event versions");
    // Release requests used to have null implementations in the server.
    wl_pointer_release(pointers[1]); wl_keyboard_release(keyboards[1]);
    wl_pointer_destroy(pointers[0]); wl_keyboard_destroy(keyboards[0]);
    struct window no_role = {0};
    create_window(&g, &no_role, 0);
    destroy_window(&no_role, 0); // Unconstructed role must not corrupt the registry.
    int fd = memfd_create("mansion-runtime-fixture", MFD_CLOEXEC);
    require(fd >= 0 && ftruncate(fd, 16) == 0, "buffer fd");
    struct wl_shm_pool *pool = wl_shm_create_pool(g.shm, fd, 16);
    struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, 2, 2, 8, WL_SHM_FORMAT_ARGB8888);
    wl_buffer_add_listener(buffer, &buffer_listener, NULL);
    wl_shm_pool_destroy(pool); close(fd);
    struct window windows[16] = {0};
    for (int i = 0; i < 16; ++i) {
        create_window(&g, &windows[i], 1);
        wl_surface_commit(windows[i].surface); // Initial commit has no buffer.
    }
    require(wl_display_roundtrip(display) >= 0, "initial configure roundtrip");
    for (int i = 0; i < 16; ++i) {
        require(windows[i].configures == 1, "one initial configure");
        wl_surface_attach(windows[i].surface, buffer, 0, 0);
        windows[i].frame = wl_surface_frame(windows[i].surface);
        wl_callback_add_listener(windows[i].frame, &frame_listener, &windows[i]);
        wl_surface_commit(windows[i].surface);
        wl_surface_commit(windows[i].surface);
    }
    require(wl_display_roundtrip(display) >= 0, "ack/buffer commit roundtrip");
    for (int i = 0; i < 16; ++i) require(windows[i].configures == 1, "commit must not cause configure loop");
    marker(argv[2], "ready"); barrier(argv[2], "go");
    int local = strcmp(mode, "destroy") != 0;
    int error_code = -1;
    struct window invalid = {0};
    if (!strcmp(mode, "stale-ack") || !strcmp(mode, "unknown-ack")) {
        xdg_surface_ack_configure(windows[0].xdg, !strcmp(mode, "stale-ack") ? windows[0].serial : 0);
        error_code = XDG_SURFACE_ERROR_INVALID_SERIAL;
    } else if (!strcmp(mode, "unconfigured") || !strcmp(mode, "roleless")) {
        create_window(&g, &invalid, !strcmp(mode, "unconfigured"));
        if (invalid.top) wl_surface_attach(invalid.surface, buffer, 0, 0);
        wl_surface_commit(invalid.surface);
        error_code = invalid.top ? XDG_SURFACE_ERROR_UNCONFIGURED_BUFFER : XDG_SURFACE_ERROR_NOT_CONSTRUCTED;
    } else if (!strcmp(mode, "bad-geometry")) {
        xdg_surface_set_window_geometry(windows[0].xdg, 0, 0, 0, 10);
        error_code = XDG_SURFACE_ERROR_INVALID_SIZE;
    }
    if (error_code >= 0) {
        require(wl_display_roundtrip(display) < 0 && wl_display_get_error(display) == EPROTO, "expected protocol rejection");
        const struct wl_interface *interface = NULL;
        require(wl_display_get_protocol_error(display, &interface, NULL) == (uint32_t)error_code &&
                interface && !strcmp(interface->name, "xdg_surface"), "exact protocol error");
    } else if (!strcmp(mode, "server-stop")) {
        require(wl_display_roundtrip(display) < 0, "server stop disconnects client");
    } else if (!strcmp(mode, "destroy")) {
        // Removing non-first registry entries used to leave the count stale.
        for (int i = 1; i < 16; i += 2) destroy_window(&windows[i], 0);
        require(wl_display_roundtrip(display) >= 0, "partial destruction");
        marker(argv[2], "partial"); barrier(argv[2], "resume");
    } else require(!strcmp(mode, "disconnect"), "known trial mode");
    if (invalid.surface) destroy_window(&invalid, 1);
    for (int i = 0; i < 16; ++i) if (local || i % 2 == 0) destroy_window(&windows[i], local);
    if (local) wl_proxy_destroy((struct wl_proxy*)buffer);
    else {
        wl_buffer_destroy(buffer);
        require(wl_display_roundtrip(display) >= 0, "final destruction");
    }
    for (int i = 0; i < 2; ++i) wl_seat_destroy(g.seats[i]);
    wl_proxy_destroy((struct wl_proxy*)g.shell);
    wl_compositor_destroy(g.compositor); wl_shm_destroy(g.shm);
    wl_registry_destroy(registry); wl_display_disconnect(display);
    marker(argv[2], "ok");
    return 0;
}
