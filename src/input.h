#pragma once

#include <wayland-server.h>

struct MansionDisplay;
struct MansionSeat;
struct MansionCompositor;

struct MansionSeat* create_seat(struct wl_display* display);
void destroy_seat(struct MansionSeat* seat);

/* Set the seat's keyboard focus. Sends leave to the old surface and enter
   to the new one. `surface` can be nullptr to clear focus. */
void seat_set_keyboard_focus(struct MansionSeat* seat,
                              struct wl_resource* surface,
                              struct MansionCompositor* comp);

/* Connect the seat to a compositor so focus can be queried. */
void compositor_set_seat(struct MansionCompositor* compositor,
                          struct MansionSeat* seat);

/* Evdev input device handling */
int input_init(void);
void input_process(void);
void input_destroy(void);
