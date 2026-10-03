#define _GNU_SOURCE
#include <wayland-client.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct globals { struct wl_compositor *compositor; struct wl_shm *shm; };
static void global(void *data, struct wl_registry *registry, uint32_t id,
                   const char *interface, uint32_t version) {
    (void)version;
    struct globals *g = data;
    if (!strcmp(interface, "wl_compositor"))
        g->compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 1);
    if (!strcmp(interface, "wl_shm"))
        g->shm = wl_registry_bind(registry, id, &wl_shm_interface, 1);
}
static void removed(void *data, struct wl_registry *registry, uint32_t name) {
    (void)data; (void)registry; (void)name;
}
static const struct wl_registry_listener listener = {global, removed};
static int marker(const char *base, const char *suffix) {
    char path[1024];
    if (snprintf(path, sizeof path, "%s.%s", base, suffix) >= (int)sizeof path) return -1;
    int fd = open(path, O_CREAT | O_WRONLY | O_CLOEXEC, 0600);
    if (fd < 0) return -1;
    close(fd);
    return 0;
}
int main(int argc, char **argv) {
    if (argc != 4) return 2;
    alarm(12);
    struct wl_display *display = wl_display_connect(argv[1]);
    if (!display) return 3;
    struct globals g = {0};
    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &listener, &g);
    if (wl_display_roundtrip(display) < 0 || !g.compositor || !g.shm) return 4;
    int fd = memfd_create("mansion-runtime-fixture", MFD_CLOEXEC);
    if (fd < 0 || ftruncate(fd, 16) < 0) return 5;
    struct wl_shm_pool *pool = wl_shm_create_pool(g.shm, fd, 16);
    struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, 2, 2, 8, WL_SHM_FORMAT_ARGB8888);
    wl_shm_pool_destroy(pool);
    close(fd);
    struct wl_surface *surfaces[16];
    struct wl_callback *callbacks[16];
    for (int i = 0; i < 16; ++i) {
        surfaces[i] = wl_compositor_create_surface(g.compositor);
        wl_surface_attach(surfaces[i], buffer, 0, 0);
        callbacks[i] = wl_surface_frame(surfaces[i]);
        wl_surface_commit(surfaces[i]);
    }
    if (wl_display_roundtrip(display) < 0 || marker(argv[2], "ready") < 0) return 6;
    char go[1024];
    snprintf(go, sizeof go, "%s.go", argv[2]);
    while (access(go, F_OK) < 0) usleep(1000);
    int expected_disconnect = !strcmp(argv[3], "server-stop");
    if (!strcmp(argv[3], "destroy")) {
        // Destroy surfaces before the shared buffer to exercise listener detachment.
        for (int i = 0; i < 16; ++i) wl_surface_destroy(surfaces[i]);
        wl_buffer_destroy(buffer);
        if (wl_display_roundtrip(display) < 0) return 7;
    } else {
        if (expected_disconnect && wl_display_roundtrip(display) >= 0) return 8;
        for (int i = 0; i < 16; ++i) wl_proxy_destroy((struct wl_proxy*)surfaces[i]);
        wl_proxy_destroy((struct wl_proxy*)buffer);
    }
    for (int i = 0; i < 16; ++i) wl_callback_destroy(callbacks[i]);
    wl_compositor_destroy(g.compositor);
    wl_shm_destroy(g.shm);
    wl_registry_destroy(registry);
    wl_display_disconnect(display);
    return marker(argv[2], "ok") == 0 ? 0 : 9;
}
