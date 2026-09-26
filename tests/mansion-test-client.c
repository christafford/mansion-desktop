/* Minimal Wayland test client for Mansion Desktop automated tests.
 *
 * Prints what it observes as single lines on stdout so shell tests can grep them:
 *   global <interface> <version>     one per advertised global (after the first roundtrip)
 *   connected                        after the registry roundtrip
 *   done                             before a clean exit
 *
 * Later tasks in docs/TASKS.md extend this program (--toplevel, --buffer, --report-input).
 * Keep the output format line-based and stable; tests depend on it.
 */
#define _GNU_SOURCE
#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <wayland-client.h>

struct client {
    struct wl_display* display;
    struct wl_registry* registry;
    struct wl_compositor* compositor;
    struct wl_shm* shm;
    struct wl_seat* seat;
    int seen_globals;
};

static void registry_global(void* data, struct wl_registry* registry, uint32_t name,
                            const char* interface, uint32_t version) {
    struct client* c = data;
    c->seen_globals++;
    printf("global %s %u\n", interface, version);
    if (strcmp(interface, wl_compositor_interface.name) == 0 && !c->compositor) {
        c->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, version < 4 ? version : 4);
    } else if (strcmp(interface, wl_shm_interface.name) == 0 && !c->shm) {
        c->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
    } else if (strcmp(interface, wl_seat_interface.name) == 0 && !c->seat) {
        c->seat = wl_registry_bind(registry, name, &wl_seat_interface, version < 4 ? version : 4);
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

static void usage(void) {
    fprintf(stderr,
            "Usage: mansion-test-client [--socket NAME] [--exit-after-ms N]\n"
            "  --socket NAME       Wayland socket name (default: $WAYLAND_DISPLAY)\n"
            "  --exit-after-ms N   keep dispatching events for N ms before exiting (default 0)\n");
}

int main(int argc, char** argv) {
    const char* socket = NULL;
    long exit_after_ms = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--socket") == 0 && i + 1 < argc) {
            socket = argv[++i];
        } else if (strcmp(argv[i], "--exit-after-ms") == 0 && i + 1 < argc) {
            exit_after_ms = strtol(argv[++i], NULL, 10);
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
    wl_registry_destroy(c.registry);
    wl_display_disconnect(c.display);
    return 0;

error:
    fprintf(stderr, "protocol error or disconnect: %s\n", strerror(errno));
    wl_display_disconnect(c.display);
    return 1;
}
