#pragma once

#include <cstdint>
#include <wayland-client.h>

// Called only for a readable host connection, on the display's owning thread.
inline int client_display_fd_handler(int, uint32_t, void* data) {
    auto* display = static_cast<wl_display*>(data);
    // dispatch pairs prepare_read/read_events internally. Calling read_events
    // alone leaves libwayland waiting for a reader that was never registered.
    wl_display_dispatch(display);
    // wl_event_loop callback returns do not propagate connection errors.
    // The main loop checks wl_display_get_error before continuing.
    return 0;
}
