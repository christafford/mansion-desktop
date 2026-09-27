#pragma once

#include <wayland-server.h>

struct MansionDisplay;
struct MansionSeat;
struct MansionCompositor;

struct MansionSeat* create_seat(struct wl_display* display);
void destroy_seat(struct MansionSeat* seat);

/* External globals for main.cpp to set. */
extern struct MansionSeat* g_seat;
extern struct MansionCompositor* g_compositor;

/* Set the seat's keyboard focus. Sends leave to the old surface and enter
   to the new one. `surface` can be nullptr to clear focus. */
void seat_set_keyboard_focus(struct MansionSeat* seat,
                              struct wl_resource* surface,
                              struct MansionCompositor* comp);

/* Connect the seat to a compositor so focus can be queried. */
void compositor_set_seat(struct MansionCompositor* compositor,
                          struct MansionSeat* seat);

/* ---------- Input script (P1-T06-E) ---------- */

/* Script state — remaining_wait_ms is exposed so main.cpp can sleep in
   small chunks between commands. */
struct InputScript {
    FILE* fp;
    int done;
    long remaining_wait_ms;
};

struct InputScript* input_script_init(const char* filename);

/* Execute the next command from the script. Returns 0 if more commands
   remain, 1 if the script requested quit, -1 on EOF (all commands done). */
int input_script_step(struct InputScript* s,
                      struct MansionSeat* seat,
                      struct MansionCompositor* comp);

/* Destroy the script handle. */
void input_script_destroy(struct InputScript* s);

/* Evdev input device handling */
int input_init(void);
void input_process(void);
void input_destroy(void);
