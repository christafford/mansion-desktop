#pragma once

#include <wayland-server.h>

struct MansionDisplay;
struct MansionSeat;

struct MansionSeat* create_seat(struct wl_display* display);
void destroy_seat(struct MansionSeat* seat);
