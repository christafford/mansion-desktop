#pragma once
#include <cstdint>
struct wl_display;
struct wl_resource;
struct MansionSeat;
struct MansionCompositor;

// Per-runtime protocol ownership. No global seat or host input backend.
MansionSeat* seat_create(wl_display* display, uint32_t version = 4);
void seat_destroy(MansionSeat* seat);
// Sends leave/enter only to the owning client; null surface clears focus.
void seat_set_keyboard_focus(MansionSeat* seat, wl_resource* surface, MansionCompositor* compositor);
// Linux evdev keycodes, with client-side repeat; duplicates are ignored.
bool seat_keyboard_key(MansionSeat* seat, uint32_t keycode, bool pressed);
// Logical surface coordinates; an implicit grab prevents target switches.
bool seat_pointer_motion(MansionSeat* seat, wl_resource* surface, double x, double y);
bool seat_pointer_button(MansionSeat* seat, uint32_t button, bool pressed);
bool seat_pointer_axis(MansionSeat* seat, double horizontal, double vertical);
void seat_pointer_reset(MansionSeat* seat); // Release held buttons, then leave.
wl_resource* seat_pointer_surface(MansionSeat* seat);
bool seat_pointer_grabbed(MansionSeat* seat);
void compositor_set_seat(MansionCompositor* compositor, MansionSeat* seat);
