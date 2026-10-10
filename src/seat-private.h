#pragma once
#include <vector>
#include <wayland-server.h>
#include <xkbcommon/xkbcommon.h>

// Shared only with the legacy input policy. No host/backend state lives here.
/* Per-client keyboard state */
struct SeatKeyboardClient {
    struct wl_resource* resource;
    struct wl_listener destroy_listener;
    struct wl_list seat_link;         /* Link in seat->keyboard_clients */
};

/* Per-client pointer state */
struct SeatPointerClient {
    struct wl_resource* resource;
    struct wl_listener destroy_listener;
    struct wl_list seat_link;         /* Link in seat->pointer_clients */
};

struct ElsewhereSeat {
    struct wl_display* display;
    struct wl_global* global;
    struct wl_list resources;
    struct wl_list keyboard_clients;   /* SeatKeyboardClient */
    struct wl_list pointer_clients;    /* SeatPointerClient */

    xkb_keymap* keymap;
    xkb_state*  xkbstate;

    /* Keyboard focus (P1-T06-C). */
    struct wl_resource* focused_surface_resource;
    struct wl_listener keyboard_focus_destroy;
    struct ElsewhereCompositor* keyboard_compositor;

    /* Pointer focus and grab (P1-T06-D). */
    struct wl_resource* pointer_surface_resource; /* surface pointer is over */
    struct wl_resource* grab_surface_resource;    /* surface with pointer grab */
    struct wl_listener pointer_focus_destroy;
    double pointer_x = 0, pointer_y = 0;
    std::vector<uint32_t> pressed_buttons;
    uint32_t serial;                              /* shared serial for all events */

    /* P3-T02: track pressed keycodes for clean exit from Application mode. */
    std::vector<uint32_t> pressed_keys;

};
