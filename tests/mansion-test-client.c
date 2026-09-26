/* Minimal Wayland test client for Mansion Desktop automated tests.
 *
 * Prints what it observes as single lines on stdout so shell tests can grep them:
 *   global <interface> <version>     one per advertised global (after the first roundtrip)
 *   connected                        after the registry roundtrip
 *   configure <w> <h>                from xdg_toplevel (when --toplevel)
 *   done                             before a clean exit
 *
 * Later tasks in docs/TASKS.md extend this program (--buffer, --color,
 * --report-input, --egl). Keep the output format line-based and stable;
 * tests depend on it.
 */
#define _GNU_SOURCE
#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

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

/* Listener structs must outlive the proxy; keep them static. */
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

static void usage(void) {
    fprintf(stderr,
            "Usage: mansion-test-client [--socket NAME] [--exit-after-ms N] [--toplevel]\n"
            "  --socket NAME       Wayland socket name (default: $WAYLAND_DISPLAY)\n"
            "  --exit-after-ms N   keep dispatching events for N ms before exiting (default 0)\n"
            "  --toplevel          create a toplevel surface and wait for configure\n");
}

int main(int argc, char** argv) {
    const char* socket = NULL;
    long exit_after_ms = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--socket") == 0 && i + 1 < argc) {
            socket = argv[++i];
        } else if (strcmp(argv[i], "--exit-after-ms") == 0 && i + 1 < argc) {
            exit_after_ms = strtol(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--toplevel") == 0) {
            /* toplevel mode is handled below */
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

    /* Check if --toplevel was requested */
    int toplevel_mode = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--toplevel") == 0) {
            toplevel_mode = 1;
            break;
        }
    }

    if (toplevel_mode) {
        /* Set up xdg-shell */
        if (!c.xdg_wm_base) {
            fprintf(stderr, "xdg_wm_base not advertised\n");
            wl_display_disconnect(c.display);
            return 1;
        }
        xdg_wm_base_add_listener(c.xdg_wm_base, &wm_base_listener, &c);

        /* Create wl_surface */
        if (!c.compositor) {
            fprintf(stderr, "wl_compositor not advertised\n");
            wl_display_disconnect(c.display);
            return 1;
        }
        struct wl_surface* surface = wl_compositor_create_surface(c.compositor);

        /* Create xdg_surface via get_xdg_surface */
        c.xdg_surface = xdg_wm_base_get_xdg_surface(c.xdg_wm_base, surface);
        if (!c.xdg_surface) {
            fprintf(stderr, "failed to create xdg_surface proxy\n");
            wl_surface_destroy(surface);
            wl_display_disconnect(c.display);
            return 1;
        }
        xdg_surface_add_listener(c.xdg_surface, &surface_listener, &c);

        /* Create xdg_toplevel */
        c.toplevel = xdg_surface_get_toplevel(c.xdg_surface);
        if (!c.toplevel) {
            fprintf(stderr, "failed to create xdg_toplevel proxy\n");
            xdg_surface_destroy(c.xdg_surface);
            wl_surface_destroy(surface);
            wl_display_disconnect(c.display);
            return 1;
        }
        xdg_toplevel_add_listener(c.toplevel, &toplevel_listener, &c);

        /* Set title and app_id (nice-to-have) */
        xdg_toplevel_set_title(c.toplevel, "test-client");
        xdg_toplevel_set_app_id(c.toplevel, "mansion-test-client");

        /* Set window geometry (optional) */
        xdg_surface_set_window_geometry(c.xdg_surface, 0, 0, 800, 600);

        /* Commit the surface to trigger configure */
        wl_surface_commit(surface);

        /* Wait for configure with timeout */
        long deadline = now_ms() + 2000;
        while (!c.received_configure && now_ms() < deadline) {
            while (wl_display_prepare_read(c.display) != 0) {
                if (wl_display_dispatch_pending(c.display) < 0) goto error;
            }
            if (wl_display_flush(c.display) < 0 && errno != EAGAIN) {
                wl_display_cancel_read(c.display);
                goto error;
            }
            struct pollfd pfd = { wl_display_get_fd(c.display), POLLIN, 0 };
            long remaining = deadline - now_ms();
            int ret = poll(&pfd, 1, remaining > 0 ? (int)remaining : 0);
            if (ret > 0) {
                if (wl_display_read_events(c.display) < 0) goto error;
                if (wl_display_dispatch_pending(c.display) < 0) goto error;
            } else {
                wl_display_cancel_read(c.display);
                if (ret < 0 && errno != EINTR) goto error;
            }
            fflush(stdout);
        }

        /* Cleanup xdg resources */
        xdg_toplevel_destroy(c.toplevel);
        xdg_surface_destroy(c.xdg_surface);
        wl_surface_destroy(surface);
    }

    /* Dispatch events until the deadline so later features can observe server events. */
    long deadline = now_ms() + exit_after_ms;
    while (now_ms() < deadline) {
        while (wl_display_prepare_read(c.display) != 0) {
            if (wl_display_dispatch_pending(c.display) < 0) goto error;
        }
        if (wl_display_flush(c.display) < 0 && errno != EAGAIN) {
            wl_display_cancel_read(c.display);
            goto error;
        }
        struct pollfd pfd = { wl_display_get_fd(c.display), POLLIN, 0 };
        long remaining = deadline - now_ms();
        int ret = poll(&pfd, 1, remaining > 0 ? (int)remaining : 0);
        if (ret > 0) {
            if (wl_display_read_events(c.display) < 0) goto error;
            if (wl_display_dispatch_pending(c.display) < 0) goto error;
        } else {
            wl_display_cancel_read(c.display);
            if (ret < 0 && errno != EINTR) goto error;
        }
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
