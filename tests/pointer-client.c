#define _GNU_SOURCE
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void check(int ok, const char *message) {
    if (!ok) { fprintf(stderr, "POINTER CLIENT FAIL: %s\n", message); exit(1); }
}
static FILE *events;
static struct wl_compositor *compositor;
static struct wl_shm *shm;
static struct xdg_wm_base *shell;
static struct wl_seat *seat;
struct pointer { int id; struct wl_pointer *proxy; };
static void enter(void *data, struct wl_pointer *p, uint32_t serial, struct wl_surface *surface, wl_fixed_t x, wl_fixed_t y) {
    (void)p; check(serial && surface, "enter serial/surface");
    fprintf(events, "%d enter %.3f %.3f\n", ((struct pointer*)data)->id, wl_fixed_to_double(x), wl_fixed_to_double(y));
}
static void leave(void *data, struct wl_pointer *p, uint32_t serial, struct wl_surface *surface) {
    (void)p; check(serial && surface, "leave serial/surface");
    fprintf(events, "%d leave\n", ((struct pointer*)data)->id);
}
static void motion(void *data, struct wl_pointer *p, uint32_t time, wl_fixed_t x, wl_fixed_t y) {
    (void)p; check(time != 0, "motion time");
    fprintf(events, "%d motion %.3f %.3f\n", ((struct pointer*)data)->id, wl_fixed_to_double(x), wl_fixed_to_double(y));
}
static void button(void *data, struct wl_pointer *p, uint32_t serial, uint32_t time, uint32_t code, uint32_t state) {
    (void)p; check(serial && time, "button serial/time");
    fprintf(events, "%d button %u %u\n", ((struct pointer*)data)->id, code, state);
}
static void axis(void *data, struct wl_pointer *p, uint32_t time, uint32_t axis, wl_fixed_t value) {
    (void)p; check(time != 0, "axis time");
    fprintf(events, "%d axis %u %.3f\n", ((struct pointer*)data)->id, axis, wl_fixed_to_double(value));
}
static const struct wl_pointer_listener pointer_listener = {
    .enter=enter, .leave=leave, .motion=motion, .button=button, .axis=axis
};
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
    struct pointer pointers[2] = {{.id=1}, {.id=2}};
    pointers[0].proxy = wl_seat_get_pointer(seat);
    wl_pointer_add_listener(pointers[0].proxy, &pointer_listener, &pointers[0]);
    int fd = memfd_create("pointer-fixture", MFD_CLOEXEC); check(fd >= 0 && ftruncate(fd, 16) == 0, "buffer fd");
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
            pointers[1].proxy = wl_seat_get_pointer(seat);
            wl_pointer_add_listener(pointers[1].proxy, &pointer_listener, &pointers[1]);
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
    for (int i=0; i<2; ++i) if (pointers[i].proxy) { wl_pointer_release(pointers[i].proxy); }
    wl_seat_destroy(seat); xdg_wm_base_destroy(shell); wl_shm_destroy(shm); wl_compositor_destroy(compositor);
    sync_client(display); wl_registry_destroy(registry); wl_display_disconnect(display); fclose(events);
    mark(argv[2], "ok");
    return 0;
}
