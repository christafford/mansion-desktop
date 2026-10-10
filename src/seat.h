#pragma once
#include <cstdint>
struct wl_display;
struct wl_resource;
struct ElsewhereSeat;
struct ElsewhereCompositor;

// Per-runtime protocol ownership. No global seat or host input backend.
ElsewhereSeat* seat_create(wl_display* display, uint32_t version = 4);
void seat_destroy(ElsewhereSeat* seat);
// Sends leave/enter only to the owning client; null surface clears focus.
void seat_set_keyboard_focus(ElsewhereSeat* seat, wl_resource* surface, ElsewhereCompositor* compositor);
// Linux evdev keycodes, with client-side repeat; duplicates are ignored.
bool seat_keyboard_key(ElsewhereSeat* seat, uint32_t keycode, bool pressed);
// Logical surface coordinates; an implicit grab prevents target switches.
bool seat_pointer_motion(ElsewhereSeat* seat, wl_resource* surface, double x, double y);
bool seat_pointer_button(ElsewhereSeat* seat, uint32_t button, bool pressed);
bool seat_pointer_axis(ElsewhereSeat* seat, double horizontal, double vertical);
void seat_pointer_reset(ElsewhereSeat* seat); // Release held buttons, then leave.
wl_resource* seat_pointer_surface(ElsewhereSeat* seat);
bool seat_pointer_grabbed(ElsewhereSeat* seat);
void compositor_set_seat(ElsewhereCompositor* compositor, ElsewhereSeat* seat);
