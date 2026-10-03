#define _GNU_SOURCE
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static void check(int ok, const char *message) {
    if (!ok) { fprintf(stderr, "FRAME CLIENT FAIL: %s\n", message); exit(1); }
}
struct client {
    struct wl_compositor *compositor;
    struct wl_shm *shm;
    struct xdg_wm_base *shell;
    int configures, frames, requested;
};
static void global(void *data, struct wl_registry *registry, uint32_t id, const char *name, uint32_t version) {
    struct client *c = data;
    if (!strcmp(name, "wl_compositor")) {
        check(version >= 4, "compositor v4");
        c->compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 4);
    } else if (!strcmp(name, "wl_shm")) c->shm = wl_registry_bind(registry, id, &wl_shm_interface, 1);
    else if (!strcmp(name, "xdg_wm_base")) c->shell = wl_registry_bind(registry, id, &xdg_wm_base_interface, 3);
}
static void removed(void *data, struct wl_registry *registry, uint32_t id) { (void)data; (void)registry; (void)id; }
static const struct wl_registry_listener registry_listener = { global, removed };
static void configure(void *data, struct xdg_surface *surface, uint32_t serial) {
    ((struct client*)data)->configures++;
    xdg_surface_ack_configure(surface, serial);
}
static const struct xdg_surface_listener xdg_listener = { configure };
static void top_configure(void *data, struct xdg_toplevel *top, int32_t width, int32_t height, struct wl_array *states) {
    (void)data; (void)top; (void)width; (void)height; (void)states;
}
static void top_close(void *data, struct xdg_toplevel *top) { (void)data; (void)top; check(0, "unexpected close"); }
static const struct xdg_toplevel_listener top_listener = { .configure = top_configure, .close = top_close };
struct frame_request { struct client *client; int sequence; };
static void done(void *data, struct wl_callback *callback, uint32_t time) {
    (void)time;
    struct frame_request *request = data;
    check(++request->client->frames == request->sequence, "frame callbacks preserve request order");
    free(request); wl_callback_destroy(callback);
}
static const struct wl_callback_listener frame_listener = { done };
static void frame(struct client *c, struct wl_surface *surface) {
    struct frame_request *request = malloc(sizeof *request); check(request != NULL, "frame context");
    request->client = c; request->sequence = ++c->requested;
    wl_callback_add_listener(wl_surface_frame(surface), &frame_listener, request);
}
struct buffer { struct wl_buffer *proxy; unsigned char *map; size_t size; int releases, fd; };
static void release(void *data, struct wl_buffer *proxy) { (void)proxy; ((struct buffer*)data)->releases++; }
static const struct wl_buffer_listener buffer_listener = { release };
static const uint32_t argb[8] = {0xffff0000, 0xff00ff00, 0xff0000ff, 0x80402010,
                                  0x00000000, 0xffffffff, 0xff123456, 0xffabcdef};
static const uint32_t xrgb[6] = {0x000a141e, 0x2828323c, 0x5546505a, 0x00646e78, 0x99828c96, 0x00a0aab4};
static void make_buffer(struct client *c, struct buffer *b, int width, int height, int stride, uint32_t format, const uint32_t *pixels) {
    b->releases = 0;
    b->size = (size_t)stride * height;
    int fd = memfd_create("mansion-frame-test", MFD_CLOEXEC);
    check(fd >= 0 && ftruncate(fd, b->size) == 0, "buffer allocation");
    b->map = mmap(NULL, b->size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    check(b->map != MAP_FAILED, "map buffer");
    memset(b->map, 0xe7, b->size);
    if (pixels) for (int y = 0; y < height; ++y)
        memcpy(b->map + (size_t)y * stride, pixels + y * width, width * 4);
    struct wl_shm_pool *pool = wl_shm_create_pool(c->shm, fd, b->size);
    b->proxy = wl_shm_pool_create_buffer(pool, 0, width, height, stride, format);
    wl_buffer_add_listener(b->proxy, &buffer_listener, b);
    wl_shm_pool_destroy(pool); b->fd = fd;
}
static void destroy_buffer(struct buffer *b) {
    wl_buffer_destroy(b->proxy); b->proxy = NULL;
    munmap(b->map, b->size); b->map = NULL; close(b->fd);
}
static void sync_client(struct wl_display *display) { check(wl_display_roundtrip(display) >= 0, "roundtrip"); }
static void barrier(const char *base, const char *stage) {
    char path[1024];
    check(snprintf(path, sizeof path, "%s.%s.ready", base, stage) < (int)sizeof path, "marker length");
    int fd = open(path, O_CREAT | O_WRONLY | O_CLOEXEC, 0600);
    check(fd >= 0, "marker"); close(fd);
    snprintf(path, sizeof path, "%s.%s.go", base, stage);
    while (access(path, F_OK) < 0) usleep(1000);
}
static void wait_frame(struct wl_display *display, struct client *c, int count, struct buffer *b) {
    while (c->frames < count || (b && !b->releases)) check(wl_display_dispatch(display) >= 0, "frame/release dispatch");
}
int main(int argc, char **argv) {
    check(argc == 4, "socket, marker, mode"); alarm(20);
    struct wl_display *display = wl_display_connect(argv[1]); check(display != NULL, "connect");
    struct client c = {0};
    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, &c); sync_client(display);
    check(c.compositor && c.shm && c.shell, "globals");
    struct wl_surface *surface = wl_compositor_create_surface(c.compositor);
    struct xdg_surface *xdg = xdg_wm_base_get_xdg_surface(c.shell, surface);
    xdg_surface_add_listener(xdg, &xdg_listener, &c);
    struct xdg_toplevel *top = xdg_surface_get_toplevel(xdg);
    xdg_toplevel_add_listener(top, &top_listener, &c);
    wl_surface_commit(surface); sync_client(display); check(c.configures == 1, "initial configure");
    if (strcmp(argv[3], "frames")) {
        // Negative buffer/state cases are rejected instead of allocating or misreading.
        if (!strcmp(argv[3], "scale")) wl_surface_set_buffer_scale(surface, 0);
        else if (!strcmp(argv[3], "transform")) wl_surface_set_buffer_transform(surface, 8);
        else {
            struct buffer bad = {0};
            if (!strcmp(argv[3], "oversize")) make_buffer(&c, &bad, 4097, 4096, 4097 * 4, WL_SHM_FORMAT_XRGB8888, NULL);
            else if (!strcmp(argv[3], "format")) make_buffer(&c, &bad, 4, 2, 24, WL_SHM_FORMAT_RGB565, NULL);
            else if (!strcmp(argv[3], "truncated")) {
                make_buffer(&c, &bad, 4, 2, 24, WL_SHM_FORMAT_ARGB8888, argb);
                sync_client(display);
                check(ftruncate(bad.fd, 0) == 0, "truncate backing file");
            }
            else if (!strcmp(argv[3], "divisibility")) {
                make_buffer(&c, &bad, 4, 2, 24, WL_SHM_FORMAT_ARGB8888, argb);
                wl_surface_set_buffer_scale(surface, 3);
            } else check(0, "unknown negative mode");
            wl_surface_attach(surface, bad.proxy, 0, 0); wl_surface_commit(surface);
            check(wl_display_roundtrip(display) < 0, "bad buffer rejected");
            wl_proxy_destroy((struct wl_proxy*)bad.proxy); munmap(bad.map, bad.size); close(bad.fd);
        }
        if (!wl_display_get_error(display)) check(wl_display_roundtrip(display) < 0, "bad state rejected");
        check(wl_display_get_error(display) == EPROTO, "expected protocol error");
        uint32_t expected = !strcmp(argv[3], "scale") ? WL_SURFACE_ERROR_INVALID_SCALE :
            !strcmp(argv[3], "transform") ? WL_SURFACE_ERROR_INVALID_TRANSFORM :
            !strcmp(argv[3], "divisibility") ? WL_SURFACE_ERROR_INVALID_SIZE :
            !strcmp(argv[3], "format") ? WL_SHM_ERROR_INVALID_FORMAT :
            !strcmp(argv[3], "truncated") ? WL_SHM_ERROR_INVALID_FD : WL_DISPLAY_ERROR_IMPLEMENTATION;
        check(wl_display_get_protocol_error(display, NULL, NULL) == expected, "exact buffer/state error code");
        wl_proxy_destroy((struct wl_proxy*)top); wl_proxy_destroy((struct wl_proxy*)xdg); wl_proxy_destroy((struct wl_proxy*)surface);
    } else {
        struct buffer a = {0}, b = {0}, skipped = {0}, d = {0}, pending = {0};
        make_buffer(&c, &a, 4, 2, 24, WL_SHM_FORMAT_ARGB8888, argb);
        wl_surface_attach(surface, a.proxy, 0, 0); frame(&c, surface); wl_surface_commit(surface); sync_client(display);
        wait_frame(display, &c, 1, &a); check(a.releases == 1, "one release");
        barrier(argv[2], "initial");
        // Client memory may be reused immediately after release.
        memset(a.map, 0xee, a.size); destroy_buffer(&a); sync_client(display);
        barrier(argv[2], "destroyed");
        make_buffer(&c, &skipped, 4, 2, 24, WL_SHM_FORMAT_ARGB8888, argb);
        make_buffer(&c, &b, 2, 3, 16, WL_SHM_FORMAT_XRGB8888, xrgb);
        wl_surface_attach(surface, skipped.proxy, 0, 0); wl_surface_attach(surface, b.proxy, 0, 0);
        frame(&c, surface); wl_surface_commit(surface); sync_client(display); wait_frame(display, &c, 2, &b);
        check(skipped.releases == 0 && b.releases == 1, "superseded pending attach not released"); destroy_buffer(&skipped);
        barrier(argv[2], "replacement");
        make_buffer(&c, &d, 4, 2, 24, WL_SHM_FORMAT_ARGB8888, argb);
        wl_surface_attach(surface, d.proxy, 0, 0); wl_surface_set_buffer_scale(surface, 2); wl_surface_set_buffer_transform(surface, 1);
        frame(&c, surface); sync_client(display);
        check(d.releases == 0 && c.frames == 2, "pending state must not be consumed");
        barrier(argv[2], "pending");
        wl_surface_commit(surface); sync_client(display); wait_frame(display, &c, 3, &d);
        barrier(argv[2], "scaled");
        // Destroying an uncommitted buffer must not erase the copied content.
        make_buffer(&c, &pending, 4, 2, 24, WL_SHM_FORMAT_ARGB8888, argb);
        wl_surface_attach(surface, pending.proxy, 0, 0); destroy_buffer(&pending);
        wl_surface_damage_buffer(surface, 0, 0, 4, 2); frame(&c, surface); wl_surface_commit(surface); sync_client(display);
        wait_frame(display, &c, 4, NULL); check(d.releases == 1, "must not release old buffer again");
        barrier(argv[2], "pending-destroyed");
        wl_surface_attach(surface, NULL, 0, 0); frame(&c, surface); wl_surface_commit(surface); sync_client(display); wait_frame(display, &c, 5, NULL);
        barrier(argv[2], "detached");
        wl_surface_set_buffer_scale(surface, 1); wl_surface_set_buffer_transform(surface, 0);
        wl_surface_commit(surface); sync_client(display); check(c.configures == 2, "remap requires a fresh configure");
        // Metadata-only commit updated the detached snapshot to revision 5.
        make_buffer(&c, &a, 4, 2, 24, WL_SHM_FORMAT_ARGB8888, argb);
        wl_surface_attach(surface, a.proxy, 0, 0); frame(&c, surface); wl_surface_commit(surface); sync_client(display); wait_frame(display, &c, 6, &a);
        barrier(argv[2], "remapped");
        frame(&c, surface); frame(&c, surface); wl_surface_commit(surface);
        sync_client(display); wait_frame(display, &c, 8, NULL);
        for (int transform = 0; transform < 8; ++transform) {
            wl_surface_set_buffer_transform(surface, transform); wl_surface_commit(surface); sync_client(display);
            char stage[24]; snprintf(stage, sizeof stage, "transform-%d", transform); barrier(argv[2], stage);
        }
        destroy_buffer(&a); destroy_buffer(&b); destroy_buffer(&d);
        xdg_toplevel_destroy(top); xdg_surface_destroy(xdg); wl_surface_destroy(surface); sync_client(display);
    }
    wl_proxy_destroy((struct wl_proxy*)c.shell); wl_shm_destroy(c.shm); wl_compositor_destroy(c.compositor);
    wl_registry_destroy(registry); wl_display_disconnect(display);
    char path[1024]; snprintf(path, sizeof path, "%s.ok", argv[2]);
    int fd = open(path, O_CREAT | O_WRONLY | O_CLOEXEC, 0600); check(fd >= 0, "completion"); close(fd);
    return 0;
}
