#define _GNU_SOURCE
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include <xkbcommon/xkbcommon.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void check(int ok, const char *message) {
    if (!ok) { fprintf(stderr, "KEYBOARD CLIENT FAIL: %s\n", message); exit(1); }
}
static FILE *events;
static struct wl_compositor *compositor;
static struct wl_shm *shm;
static struct xdg_wm_base *shell;
static struct wl_seat *seat;
struct keyboard { int id; struct xkb_state *state; struct wl_keyboard *proxy; };
static void keymap(void *data, struct wl_keyboard *proxy, uint32_t format, int fd, uint32_t size) {
    (void)proxy;
    check(format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1 && size > 1, "keymap format");
    char *text = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    check(text != MAP_FAILED && text[size - 1] == 0, "keymap memory");
    struct xkb_context *ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    check(ctx != NULL, "xkb context");
    struct xkb_keymap *map = xkb_keymap_new_from_string(ctx, text, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
    check(map != NULL, "keymap parse");
    struct keyboard *k = data;
    k->state = xkb_state_new(map);
    check(k->state != NULL, "xkb state");
    xkb_keymap_unref(map); xkb_context_unref(ctx); munmap(text, size); close(fd);
}
static void enter(void *data, struct wl_keyboard *proxy, uint32_t serial, struct wl_surface *surface, struct wl_array *keys) {
    (void)proxy; (void)surface;
    check(serial != 0, "enter serial");
    fprintf(events, "%d enter %zu\n", ((struct keyboard*)data)->id, keys->size / sizeof(uint32_t));
    uint32_t *held;
    wl_array_for_each(held, keys) fprintf(events, "%d held %u\n", ((struct keyboard*)data)->id, *held);
}
static void leave(void *data, struct wl_keyboard *proxy, uint32_t serial, struct wl_surface *surface) {
    (void)proxy; (void)surface; check(serial != 0, "leave serial");
    fprintf(events, "%d leave\n", ((struct keyboard*)data)->id);
}
static void key(void *data, struct wl_keyboard *proxy, uint32_t serial, uint32_t time, uint32_t code, uint32_t state) {
    (void)proxy;
    check(serial != 0 && time != 0, "key serial/time");
    struct keyboard *k = data;
    fprintf(events, "%d key %u %u %u\n", k->id, code, state, xkb_state_key_get_one_sym(k->state, code + 8));
}
static void modifiers(void *data, struct wl_keyboard *proxy, uint32_t serial,
                      uint32_t depressed, uint32_t latched, uint32_t locked, uint32_t group) {
    (void)proxy; check(serial != 0, "modifier serial");
    struct keyboard *k = data;
    xkb_state_update_mask(k->state, depressed, latched, locked, 0, 0, group);
    fprintf(events, "%d mods %u %u %u %u\n", k->id, depressed, latched, locked, group);
}
static void repeat(void *data, struct wl_keyboard *proxy, int32_t rate, int32_t delay) {
    (void)data; (void)proxy; check(rate == 25 && delay == 500, "repeat settings");
}
static const struct wl_keyboard_listener keyboard_listener = {keymap, enter, leave, key, modifiers, repeat};
static void global(void *data, struct wl_registry *registry, uint32_t id, const char *name, uint32_t version) {
    (void)data; (void)version;
    if (!strcmp(name, "wl_compositor")) compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 4);
    else if (!strcmp(name, "wl_shm")) shm = wl_registry_bind(registry, id, &wl_shm_interface, 1);
    else if (!strcmp(name, "xdg_wm_base")) shell = wl_registry_bind(registry, id, &xdg_wm_base_interface, 3);
    else if (!strcmp(name, "wl_seat")) seat = wl_registry_bind(registry, id, &wl_seat_interface, 4);
}
static void removed(void *data, struct wl_registry *r, uint32_t id) { (void)data; (void)r; (void)id; }
static const struct wl_registry_listener registry_listener = {global, removed};
static void configure(void *data, struct xdg_surface *s, uint32_t serial) { (void)data; xdg_surface_ack_configure(s, serial); }
static const struct xdg_surface_listener surface_listener = {configure};
static void top_configure(void *data, struct xdg_toplevel *t, int32_t w, int32_t h, struct wl_array *states) {
    (void)data; (void)t; (void)w; (void)h; (void)states;
}
static void top_close(void *data, struct xdg_toplevel *t) { (void)data; (void)t; check(0, "unexpected close"); }
static const struct xdg_toplevel_listener top_listener = {.configure=top_configure, .close=top_close};
static int exists(const char *base, const char *suffix) {
    char path[2048]; check(snprintf(path, sizeof path, "%s.%s", base, suffix) < (int)sizeof path, "path");
    return access(path, F_OK) == 0;
}
static void mark(const char *base, const char *suffix) {
    char path[2048]; check(snprintf(path, sizeof path, "%s.%s", base, suffix) < (int)sizeof path, "path");
    int fd = open(path, O_CREAT | O_WRONLY | O_CLOEXEC, 0600); check(fd >= 0, "marker"); close(fd);
}
static void sync_client(struct wl_display *display) { check(wl_display_roundtrip(display) >= 0, "roundtrip"); }
int main(int argc, char **argv) {
    check(argc == 3, "socket and marker base"); alarm(30);
    char path[2048]; check(snprintf(path, sizeof path, "%s.events", argv[2]) < (int)sizeof path, "log path");
    events = fopen(path, "w"); check(events != NULL, "event log"); setvbuf(events, NULL, _IOLBF, 0);
    struct wl_display *display = wl_display_connect(argv[1]); check(display != NULL, "connect");
    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL); sync_client(display);
    check(compositor && shm && shell && seat, "globals");
    struct keyboard keyboards[2] = {{.id=1}, {.id=2}};
    keyboards[0].proxy = wl_seat_get_keyboard(seat);
    wl_keyboard_add_listener(keyboards[0].proxy, &keyboard_listener, &keyboards[0]);
    int fd = memfd_create("keyboard-fixture", MFD_CLOEXEC); check(fd >= 0 && ftruncate(fd, 16) == 0, "buffer fd");
    struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, 16);
    struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, 2, 2, 8, WL_SHM_FORMAT_XRGB8888);
    wl_shm_pool_destroy(pool); close(fd);
    struct wl_surface *surfaces[2]; struct xdg_surface *xdgs[2]; struct xdg_toplevel *tops[2];
    for (int i=0; i<2; ++i) {
        surfaces[i] = wl_compositor_create_surface(compositor);
        xdgs[i] = xdg_wm_base_get_xdg_surface(shell, surfaces[i]);
        xdg_surface_add_listener(xdgs[i], &surface_listener, NULL);
        tops[i] = xdg_surface_get_toplevel(xdgs[i]); xdg_toplevel_add_listener(tops[i], &top_listener, NULL);
        wl_surface_commit(surfaces[i]);
    }
    sync_client(display);
    for (int i=0; i<2; ++i) { wl_surface_attach(surfaces[i], buffer, 0, 0); wl_surface_commit(surfaces[i]); }
    struct wl_surface *roleless = wl_compositor_create_surface(compositor);
    sync_client(display); mark(argv[2], "ready");
    int detached=0, late=0, destroyed=0;
    while (!exists(argv[2], "finish")) {
        if (!late && exists(argv[2], "late")) {
            keyboards[1].proxy = wl_seat_get_keyboard(seat);
            wl_keyboard_add_listener(keyboards[1].proxy, &keyboard_listener, &keyboards[1]);
            sync_client(display); mark(argv[2], "late-ok"); late=1;
        }
        if (!detached && exists(argv[2], "detach")) {
            for (int i=0; i<2; ++i) { wl_surface_attach(surfaces[i], NULL, 0, 0); wl_surface_commit(surfaces[i]); }
            sync_client(display); mark(argv[2], "detach-ok"); detached=1;
        }
        if (!destroyed && exists(argv[2], "destroy")) {
            for (int i=0; i<2; ++i) { xdg_toplevel_destroy(tops[i]); xdg_surface_destroy(xdgs[i]); wl_surface_destroy(surfaces[i]); }
            sync_client(display); mark(argv[2], "destroy-ok"); destroyed=1;
        }
        sync_client(display); usleep(1000);
    }
    if (!destroyed) for (int i=0; i<2; ++i) { xdg_toplevel_destroy(tops[i]); xdg_surface_destroy(xdgs[i]); wl_surface_destroy(surfaces[i]); }
    wl_surface_destroy(roleless); wl_buffer_destroy(buffer);
    for (int i=0; i<2; ++i) if (keyboards[i].proxy) { wl_keyboard_release(keyboards[i].proxy); xkb_state_unref(keyboards[i].state); }
    wl_seat_destroy(seat); xdg_wm_base_destroy(shell); wl_shm_destroy(shm); wl_compositor_destroy(compositor);
    sync_client(display); wl_registry_destroy(registry); wl_display_disconnect(display); fclose(events);
    mark(argv[2], "ok");
    return 0;
}
