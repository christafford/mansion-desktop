#pragma once

#include <wayland-server.h>

struct MansionDisplay;
struct MansionSeat;
struct MansionCompositor;

struct MansionSeat* create_seat(struct wl_display* display);
void destroy_seat(struct MansionSeat* seat);

/* ---------- Input mode (P3-T01) ---------- */

enum class InputMode { World, Application };

/* Set the global input mode. In World mode, input drives the camera
 * (clients receive nothing). In Application mode, input goes to the
 * focused surface. */
void input_mode_set(InputMode mode);
InputMode input_mode_get(void);

/* P3-T02: set/get the world-key evdev code. */
void input_world_key_set(int code);
int input_world_key_get(void);

/* P4-T02: set/get the teleport-key evdev code. */
void input_teleport_key_set(int code);
int input_teleport_key_get(void);

/* P5-T02: set/get the tab-key evdev code. */
void input_tab_key_set(int code);
int input_tab_key_get(void);

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

/* P3-T06: Clear all seat-level focus pointers to prevent dangling references
 * when a focused surface is destroyed. */
void seat_clear_focus_state(void);

/* ---------- Input script (P1-T06-E) ---------- */

/* Camera movement state (P2-T06). */
struct MovementState {
    bool w = false, a = false, s = false, d = false;
    bool up = false, down = false, left = false, right = false;
    bool right_button_pressed = false;
    double mouseX = 0, mouseY = 0;
};

/* Script state — remaining_wait_ms is exposed so main.cpp can sleep in
   small chunks between commands. */
struct InputScript {
    FILE* fp;
    int done;
    long remaining_wait_ms;
    MovementState movement;  /* P2-T06: WASD movement + mouse look */
};

struct InputScript* input_script_init(const char* filename);

/* Execute the next command from the script. Returns 0 if more commands
   remain, 1 if the script requested quit, -1 on EOF (all commands done). */
int input_script_step(struct InputScript* s,
                      struct MansionSeat* seat,
                      struct MansionCompositor* comp,
                      struct MansionDisplay* display);

/* Apply accumulated movement / mouse look to the camera.
   Called from the main render loop with the frame delta. */
void input_script_apply_movement(struct InputScript* s,
                                 struct MansionDisplay* display,
                                 double delta_ms);

/* Destroy the script handle. */
void input_script_destroy(struct InputScript* s);

/* Evdev input device handling (now a no-op; replaced by Wayland client
 * input in windowed mode). */
int input_init(void);
void input_process(void);
void input_destroy(void);

/* Handle window close request from the Wayland client (xdg_toplevel.close). */
void input_handle_window_close(void);

/* Process Wayland client events (dispatch pending events, check close flag).
 * Returns 0 on success, -1 if the window was closed by the user. */
int input_process_wayland_client(struct MansionDisplay* m_display);

/* Wayland seat input callbacks — called from the Wayland client
 * display event handlers to drive camera movement in windowed mode. */
void input_wayland_focus(bool focused);
void input_wayland_key(uint32_t key, bool pressed);
void input_wayland_pointer_motion(double dx, double dy);
void input_wayland_apply_movement(struct MansionDisplay* display, double delta_ms);
