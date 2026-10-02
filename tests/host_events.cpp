#include "src/host-events.h"

#include <cassert>
#include <sys/socket.h>
#include <unistd.h>
#include <wayland-server.h>

static void sync_done(void* data, wl_callback* callback, uint32_t) {
    *static_cast<bool*>(data) = true;
    wl_callback_destroy(callback);
}

int main() {
    // Bound a regression deadlock even outside Meson's timeout wrapper.
    alarm(3);
    int sockets[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sockets) == 0);
    auto* server = wl_display_create();
    assert(server);
    assert(wl_client_create(server, sockets[0]));
    auto* client = wl_display_connect_to_fd(sockets[1]);
    assert(client);
    auto* loop = wl_display_get_event_loop(server);
    auto* source = wl_event_loop_add_fd(loop, wl_display_get_fd(client),
        WL_EVENT_READABLE, client_display_fd_handler, client);
    assert(source);

    const wl_callback_listener listener = {sync_done};
    for (int i = 0; i < 3; ++i) {
        bool done = false;
        auto* callback = wl_display_sync(client);
        assert(wl_callback_add_listener(callback, &listener, &done) == 0);
        assert(wl_display_flush(client) >= 0);
        assert(wl_event_loop_dispatch(loop, 0) == 0);
        wl_display_flush_clients(server);
        assert(wl_event_loop_dispatch(loop, 0) == 0);
        assert(done);
        assert(wl_display_get_error(client) == 0);
    }

    // A disconnected host must return control and expose an error, not hang.
    wl_display_destroy_clients(server);
    assert(wl_event_loop_dispatch(loop, 0) == 0);
    assert(wl_display_get_error(client) != 0);
    wl_event_source_remove(source);
    wl_display_disconnect(client);
    wl_display_destroy(server);
}
