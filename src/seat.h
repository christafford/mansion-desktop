#pragma once
struct wl_display;
struct wl_resource;
struct MansionSeat;
struct MansionCompositor;

// Per-runtime protocol ownership. No global seat or host input backend.
MansionSeat* seat_create(wl_display* display);
void seat_destroy(MansionSeat* seat);
// Sends leave/enter only to the owning client; null surface clears focus.
void seat_set_keyboard_focus(MansionSeat* seat, wl_resource* surface, MansionCompositor* compositor);
void compositor_set_seat(MansionCompositor* compositor, MansionSeat* seat);
