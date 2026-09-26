/* Minimal Wayland test client for Mansion Desktop automated tests.
 *
 * Prints what it observes as single lines on stdout so shell tests can grep them:
 *   global <interface> <version>     one per advertised global (after the first roundtrip)
 *   connected                        after the registry roundtrip
 *   configure <w> <h>                from xdg_toplevel (when --toplevel)
 *   frame <time_ms>                  from wl_surface.frame callback (when --buffer)
 *   release                          when wl_buffer.release arrives (when --buffer)
 *   done                             before a clean exit
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>

#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"

struct client {
    struct wl_display* display;
    struct wl_registry* registry;
    struct wl_compositor* compositor;
    struct wl_shm* shm;
    struct wl_seat* seat;

    /* xdg-shell */
    struct xdg_wm_base* xdg_wm_base;
    struct xdg_surface* xdg_surface;
    struct xdg_toplevel* toplevel;
    int configured;
    int received_configure;

    /* buffer mode */
    struct wl_surface* surface;
    int have_buffer;
    int frame_done;
    int got_release;
    int buffer_width, buffer_height;
    int color_r, color_g, color_b;
    long frame_time;
};

static void registry_global(void* data, struct wl_registry* registry, uint32_t name,
                            const char* interface, uint32_t version) {
    struct client* c = data;
    printf("global %s %u\n", interface, version);
    if (strcmp(interface, wl_compositor_interface.name) == 0 && !c->compositor) {
        c->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, version < 4 ? version : 4);
    } else if (strcmp(interface, wl_shm_interface.name) == 0 && !c->shm) {
        c->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
    } else if (strcmp(interface, wl_seat_interface.name) == 0 && !c->seat) {
        c->seat = wl_registry_bind(registry, name, &wl_seat_interface, version < 4 ? version : 4);
    } else if (strcmp(interface, "xdg_wm_base") == 0 && !c->xdg_wm_base) {
        c->xdg_wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface, version < 3 ? version : 3);
    }
}

static void registry_global_remove(void* data, struct wl_registry* registry, uint32_t name) {
    (void)data; (void)registry;
    printf("global_remove %u\n", name);
}

static const struct wl_registry_listener registry_listener = {
    registry_global,
    registry_global_remove,
};

static long now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000L + ts.tv_nsec / 1000000L;
}

/* xdg-wm-base event handler */
static void xdg_wm_base_ping(void* data, struct xdg_wm_base* wm, uint32_t serial) {
    (void)data; (void)wm;
    xdg_wm_base_pong(wm, serial);
}

static const struct xdg_wm_base_listener wm_base_listener = {
    .ping = xdg_wm_base_ping,
};

/* xdg-surface configure handler */
static void xdg_surface_configure(void* data, struct xdg_surface* surface, uint32_t serial) {
    struct client* c = data; (void)surface;
    c->configured = 1;
    xdg_surface_ack_configure(surface, serial);
}

static const struct xdg_surface_listener surface_listener = {
    .configure = xdg_surface_configure,
};

/* xdg-toplevel configure handler */
static void xdg_toplevel_configure(void* data, struct xdg_toplevel* toplevel,
                                    int32_t width, int32_t height, struct wl_array* states) {
    struct client* c = data; (void)toplevel; (void)states;
    if (!c->received_configure) {
        c->received_configure = 1;
        printf("configure %d %d\n", width, height);
    }
}

static const struct xdg_toplevel_listener toplevel_listener = {
    .configure = xdg_toplevel_configure,
};

/* ---------- frame callback ---------- */

static void frame_done(void* data, struct wl_callback* cb, uint32_t time_ms) {
    struct client* c = data; (void)cb;
    c->frame_done = 1;
    c->frame_time = time_ms;
    printf("frame %u\n", time_ms);
    fflush(stdout);
}

static const struct wl_callback_listener callback_listener = {
    .done = frame_done,
};

/* ---------- wl_buffer release ---------- */

static void buffer_release(void* data, struct wl_buffer* buffer) {
    struct client* c = data; (void)buffer;
    c->got_release = 1;
    printf("release\n");
    fflush(stdout);
}

static const struct wl_buffer_listener buffer_listener = {
    .release = buffer_release,
};

static void usage(void) {
    fprintf(stderr,
            "Usage: mansion-test-client [--socket NAME] [--exit-after-ms N] "
            "[--toplevel] [--buffer WxH --color RRGGBB]\n"
            "  --socket NAME       Wayland socket name (default: $WAYLAND_DISPLAY)\n"
            "  --exit-after-ms N   keep dispatching events for N ms before exiting (default 0)\n"
            "  --toplevel          create a toplevel surface and wait for configure\n"
            "  --buffer WxH        create a WxH ARGB8888 shm buffer (implies --toplevel)\n"
            "  --color RRGGBB      fill buffer with color (default ff0000 = red)\n");
}

/* Create a shm pool, map it, fill with color, create buffer, attach to surface. */
static void setup_buffer(struct client* c, int w, int h, int r, int g, int b) {
    int32_t stride = w * 4;
    int32_t size = stride * h;

    int fd = memfd_create("mansion-shm", 0);
    if (fd < 0) {
        perror("memfd_create");
        return;
    }
    if (ftruncate(fd, size) < 0) {
        perror("ftruncate");
        close(fd);
        return;
    }

    void* ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return;
    }

    /* Fill with color (ARGB8888: alpha=255). */
    uint32_t* pixels = (uint32_t*)ptr;
    for (int i = 0; i < w * h; i++) {
        pixels[i] = (255u << 24) | ((b & 0xff) << 16) | ((g & 0xff) << 8) | (r & 0xff);
    }

    struct wl_shm_pool* pool = wl_shm_create_pool(c->shm, fd, size);
    struct wl_buffer* buffer = wl_shm_pool_create_buffer(pool, 0, w, h, stride, WL_SHM_FORMAT_ARGB8888);
    wl_shm_pool_destroy(pool);
    close(fd);

    munmap(ptr, size);

    wl_buffer_add_listener(buffer, &buffer_listener, c);

    c->buffer_width = w;
    c->buffer_height = h;
    wl_surface_attach(c->surface, buffer, 0, 0);
    wl_surface_damage(c->surface, 0, 0, w, h);
    wl_surface_commit(c->surface);

    /* Request a frame callback so the client can verify rendering timing. */
    struct wl_callback* cb = wl_surface_frame(c->surface);
    wl_callback_add_listener(cb, &callback_listener, c);

    c->have_buffer = 1;
}

/* Dispatch events with a timeout, returning 0 on success, -1 on disconnect. */
static int dispatch_with_timeout(struct client* c, long deadline) {
    while (wl_display_prepare_read(c->display) != 0) {
        if (wl_display_dispatch_pending(c->display) < 0) return -1;
    }
    if (wl_display_flush(c->display) < 0 && errno != EAGAIN) {
        wl_display_cancel_read(c->display);
        return -1;
    }
    struct pollfd pfd = { wl_display_get_fd(c->display), POLLIN, 0 };
    long remaining = deadline - now_ms();
    int ret = poll(&pfd, 1, remaining > 0 ? (int)remaining : 0);
    if (ret > 0) {
        if (wl_display_read_events(c->display) < 0) return -1;
        if (wl_display_dispatch_pending(c->display) < 0) return -1;
    } else {
        wl_display_cancel_read(c->display);
        if (ret < 0 && errno != EINTR) return -1;
    }
    return 0;
}

int main(int argc, char** argv) {
    const char* socket = NULL;
    long exit_after_ms = 0;

    /* Parse --buffer and --color early (they also imply --toplevel). */
    int buffer_mode = 0;
    int bw = 200, bh = 100;
    int cr = 0xff, cg = 0x00, cb = 0x00; /* default red */

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--socket") == 0 && i + 1 < argc) {
            socket = argv[++i];
        } else if (strcmp(argv[i], "--exit-after-ms") == 0 && i + 1 < argc) {
            exit_after_ms = strtol(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--toplevel") == 0) {
            /* handled below */
        } else if (strcmp(argv[i], "--buffer") == 0 && i + 1 < argc) {
            buffer_mode = 1;
            if (sscanf(argv[++i], "%dx%d", &bw, &bh) != 2) {
                fprintf(stderr, "invalid --buffer size: %s\n", argv[i]);
                return 2;
            }
        } else if (strcmp(argv[i], "--color") == 0 && i + 1 < argc) {
            unsigned int color;
            if (sscanf(argv[++i], "%x", &color) != 1) {
                fprintf(stderr, "invalid --color: %s\n", argv[i]);
                return 2;
            }
            cr = (color >> 16) & 0xff;
            cg = (color >> 8) & 0xff;
            cb = color & 0xff;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            usage();
            return 0;
        } else {
            fprintf(stderr, "unknown argument: %s\n", argv[i]);
            usage();
            return 2;
        }
    }

    struct client c = {0};
    c.display = wl_display_connect(socket);
    if (!c.display) {
        fprintf(stderr, "connect failed (socket %s): %s\n", socket ? socket : "$WAYLAND_DISPLAY", strerror(errno));
        return 1;
    }

    c.registry = wl_display_get_registry(c.display);
    wl_registry_add_listener(c.registry, &registry_listener, &c);
    if (wl_display_roundtrip(c.display) < 0) {
        fprintf(stderr, "roundtrip failed: %s\n", strerror(errno));
        return 1;
    }
    printf("connected\n");
    fflush(stdout);

    int toplevel_mode = 0;
    /* Check if --toplevel was requested or implied by --buffer */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--toplevel") == 0) {
            toplevel_mode = 1;
            break;
        }
    }
    if (buffer_mode) toplevel_mode = 1;

    if (toplevel_mode) {
        if (!c.xdg_wm_base) {
            fprintf(stderr, "xdg_wm_base not advertised\n");
            wl_display_disconnect(c.display);
            return 1;
        }
        xdg_wm_base_add_listener(c.xdg_wm_base, &wm_base_listener, &c);

        if (!c.compositor) {
            fprintf(stderr, "wl_compositor not advertised\n");
            wl_display_disconnect(c.display);
            return 1;
        }
        c.surface = wl_compositor_create_surface(c.compositor);

        c.xdg_surface = xdg_wm_base_get_xdg_surface(c.xdg_wm_base, c.surface);
        if (!c.xdg_surface) {
            fprintf(stderr, "failed to create xdg_surface proxy\n");
            wl_surface_destroy(c.surface);
            wl_display_disconnect(c.display);
            return 1;
        }
        xdg_surface_add_listener(c.xdg_surface, &surface_listener, &c);

        c.toplevel = xdg_surface_get_toplevel(c.xdg_surface);
        if (!c.toplevel) {
            fprintf(stderr, "failed to create xdg_toplevel proxy\n");
            xdg_surface_destroy(c.xdg_surface);
            wl_surface_destroy(c.surface);
            wl_display_disconnect(c.display);
            return 1;
        }
        xdg_toplevel_add_listener(c.toplevel, &toplevel_listener, &c);

        xdg_toplevel_set_title(c.toplevel, "test-client");
        xdg_toplevel_set_app_id(c.toplevel, "mansion-test-client");
        xdg_surface_set_window_geometry(c.xdg_surface, 0, 0, bw, bh);

        if (buffer_mode) {
            /* Commit with buffer to trigger configure */
            setup_buffer(&c, bw, bh, cr, cg, cb);
        } else {
            wl_surface_commit(c.surface);
        }

        /* Wait for configure with timeout */
        long deadline = now_ms() + 2000;
        while (!c.received_configure && now_ms() < deadline) {
            if (dispatch_with_timeout(&c, deadline) < 0) goto error;
        }

        if (buffer_mode) {
            /* Wait for frame callback with timeout */
            long frame_deadline = now_ms() + 2000;
            while (!c.frame_done && now_ms() < frame_deadline) {
                if (dispatch_with_timeout(&c, frame_deadline) < 0) goto error;
            }

            /* Wait for buffer release with timeout */
            long rel_deadline = now_ms() + 2000;
            while (!c.got_release && now_ms() < rel_deadline) {
                if (dispatch_with_timeout(&c, rel_deadline) < 0) goto error;
            }

            /* Cleanup xdg resources — do NOT destroy buffer/surface yet;
             * the buffer release handler may still want to print "release". */
            xdg_toplevel_destroy(c.toplevel);
            xdg_surface_destroy(c.xdg_surface);
            /* Let the compositor destroy the buffer after release. */
        } else {
            xdg_toplevel_destroy(c.toplevel);
            xdg_surface_destroy(c.xdg_surface);
            wl_surface_destroy(c.surface);
        }
    }

    /* Dispatch events until the deadline so later features can observe server events. */
    long deadline = now_ms() + exit_after_ms;
    while (now_ms() < deadline) {
        if (dispatch_with_timeout(&c, deadline) < 0) goto error;
        fflush(stdout);
    }

    printf("done\n");
    fflush(stdout);
    if (c.seat) wl_seat_destroy(c.seat);
    if (c.shm) wl_shm_destroy(c.shm);
    if (c.compositor) wl_compositor_destroy(c.compositor);
    if (c.xdg_wm_base) xdg_wm_base_destroy(c.xdg_wm_base);
    wl_registry_destroy(c.registry);
    wl_display_disconnect(c.display);
    return 0;

error:
    fprintf(stderr, "protocol error or disconnect: %s\n", strerror(errno));
    wl_display_disconnect(c.display);
    return 1;
}
