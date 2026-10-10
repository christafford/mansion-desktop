#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <vector>
#include <algorithm>
#include <iostream>
#include <unistd.h>
#include <linux/input.h>

#include <wayland-server-protocol.h>
#include <xkbcommon/xkbcommon.h>

#include "compositor.h"
#include "compositor-private.h"
#include "display.h"
#include "input.h"
#include "seat-private.h"
#include "room.h"
#include "xdg-shell.h"

/* Current pointer state */
static wl_fixed_t pointer_x = wl_fixed_from_int(300);
static wl_fixed_t pointer_y = wl_fixed_from_int(200);

/* ---------- Input mode (P3-T01) ---------- */
static InputMode g_input_mode = InputMode::World;

/* P3-T02: evdev keycode for switching to world mode (default: F12 = 88). */
static int g_world_key = 88;

/* P4-T02: evdev keycode for teleport (default: T = 20). */
static int g_teleport_key = 0;

/* P5-T02: evdev keycode for tab key (default: Tab = 23). */
static int g_tab_key = 23;

/* Movement key codes (from linux/input.h). */
static constexpr double MOVEMENT_SPEED = 0.01;
static constexpr double MOUSE_SENSITIVITY = 0.002;

/* Input belongs to the focused nested host window, never global devices. */
static MovementState live_movement;
static bool live_focused = false;

void input_mode_set(InputMode mode) {
    if (mode != g_input_mode) live_movement = {};
    g_input_mode = mode;
}

InputMode input_mode_get(void) {
    return g_input_mode;
}

void input_world_key_set(int code) {
    g_world_key = code;
}

int input_world_key_get(void) {
    return g_world_key;
}

void input_teleport_key_set(int code) {
    g_teleport_key = code;
}

int input_teleport_key_get(void) {
    return g_teleport_key;
}

void input_tab_key_set(int code) {
    g_tab_key = code;
}

int input_tab_key_get(void) {
    return g_tab_key;
}

/* Global pointer to seat for input forwarding */
struct ElsewhereSeat* g_seat = nullptr;
/* Global pointer to compositor for pointer hit testing. */
struct ElsewhereCompositor* g_compositor = nullptr;

static InputMode stored_mode_when_focus_lost = InputMode::World;

int input_init(void) { return 0; }
void input_destroy(void) { input_wayland_focus(false); }

/* P3-T06: Clear all seat-level focus pointers to prevent dangling references
 * when a focused surface is destroyed. */
void seat_clear_focus_state(void) {
    if (!g_seat) return;
    g_seat->focused_surface_resource = nullptr;
    g_seat->pointer_surface_resource = nullptr;
}

/* Iterate over all pointer clients and send motion events. */
static void send_pointer_motion(uint32_t time_msec) {
    if (!g_seat) return;

    SeatPointerClient *pk, *pk_next;
    wl_list_for_each_safe(pk, pk_next, &g_seat->pointer_clients, seat_link) {
        if (pk->resource) {
            wl_pointer_send_motion(pk->resource, time_msec,
                                   pointer_x, pointer_y);
        }
    }
}

/* Iterate over all pointer clients and send wheel events. */
static void send_pointer_axis(uint32_t time_msec, enum wl_pointer_axis axis,
                              wl_fixed_t value) {
    if (!g_seat) return;

    SeatPointerClient *pk, *pk_next;
    wl_list_for_each_safe(pk, pk_next, &g_seat->pointer_clients, seat_link) {
        if (pk->resource) {
            wl_pointer_send_axis(pk->resource, time_msec, axis, value);
        }
    }
}

/* Iterate over all pointer clients and send button events. */
static void send_pointer_button(uint32_t serial, uint32_t time_msec,
                                uint32_t button, uint32_t state) {
    if (!g_seat) return;

    SeatPointerClient *pk, *pk_next;
    wl_list_for_each_safe(pk, pk_next, &g_seat->pointer_clients, seat_link) {
        if (pk->resource) {
            wl_pointer_send_button(pk->resource, serial, time_msec,
                                   button, state);
        }
    }
}

/* Iterate over all pointer clients and send axis-done. */
static void send_pointer_axis_done(uint32_t time_msec) {
    (void)time_msec;
    // wl_pointer v4 doesn't have axis_done; it's a v6+ extension.
}

/* Send modifier state to all keyboard clients. */
static void send_keymap_modifiers(uint32_t serial) {
    if (!g_seat || !g_seat->xkbstate) return;

    xkb_mod_mask_t depressed =
        xkb_state_serialize_mods(g_seat->xkbstate, XKB_STATE_MODS_DEPRESSED);
    xkb_mod_mask_t latched =
        xkb_state_serialize_mods(g_seat->xkbstate, XKB_STATE_MODS_LATCHED);
    xkb_mod_mask_t locked =
        xkb_state_serialize_mods(g_seat->xkbstate, XKB_STATE_MODS_LOCKED);
    xkb_layout_index_t group =
        xkb_state_serialize_layout(g_seat->xkbstate, XKB_STATE_LAYOUT_EFFECTIVE);

    SeatKeyboardClient *kc, *kc_next;
    wl_list_for_each_safe(kc, kc_next, &g_seat->keyboard_clients, seat_link) {
        if (kc->resource) {
            wl_keyboard_send_modifiers(kc->resource, serial,
                                       depressed, latched, locked, group);
        }
    }
}

/* P3-T02: Cleanly exit Application mode — send key releases, empty modifiers,
 * keyboard leave, and pointer leave.  Does NOT change the input mode itself
 * (P3-T03 saves the mode for restore on focus regain; P3-T02 world-key
 * handler changes the mode explicitly). */
static void exit_application_mode(void) {
    if (!g_seat) return;

    uint32_t serial = ++g_seat->serial;

    /* Send key release for every pressed key. */
    for (uint32_t key : g_seat->pressed_keys) {
        SeatKeyboardClient *kc, *kc_next;
        wl_list_for_each_safe(kc, kc_next, &g_seat->keyboard_clients, seat_link) {
            if (kc->resource)
                wl_keyboard_send_key(kc->resource, serial, 0, key,
                    WL_KEYBOARD_KEY_STATE_RELEASED);
        }
    }
    g_seat->pressed_keys.clear();

    /* Send empty modifiers. */
    send_keymap_modifiers(serial);

    /* Send keyboard leave.
     * Only send to clients whose resource belongs to the same client
     * as the focused surface — libwayland rejects cross-client pointers. */
    if (g_seat->focused_surface_resource) {
        SeatKeyboardClient *kc, *kc_next;
        struct wl_client *old_client = wl_resource_get_client(
            g_seat->focused_surface_resource);
        wl_list_for_each_safe(kc, kc_next, &g_seat->keyboard_clients, seat_link) {
            if (kc->resource &&
                wl_resource_get_client(kc->resource) == old_client)
                wl_keyboard_send_leave(kc->resource, serial,
                    g_seat->focused_surface_resource);
        }
        g_seat->focused_surface_resource = nullptr;
    }

    /* Send pointer leave.
     * Only send to clients whose resource belongs to the same client
     * as the pointer surface. */
    if (g_seat->pointer_surface_resource) {
        SeatPointerClient *pk, *pk_next;
        struct wl_client *surf_client = wl_resource_get_client(
            g_seat->pointer_surface_resource);
        wl_list_for_each_safe(pk, pk_next, &g_seat->pointer_clients, seat_link) {
            if (pk->resource &&
                wl_resource_get_client(pk->resource) == surf_client)
                wl_pointer_send_leave(pk->resource, serial,
                    g_seat->pointer_surface_resource);
        }
        g_seat->pointer_surface_resource = nullptr;
    }

    /* NOTE: Does NOT change input_mode — caller sets the mode explicitly. */
}

/* ---------- Pointer hit test (P1-T06-D) ---------- */

/* Hit-test surfaces in z-order (top-to-bottom = reverse of surface_list)
   and return the topmost surface whose rectangle contains (x, y). */
static struct wl_resource* pointer_hit_test(wl_fixed_t x, wl_fixed_t y) {
    if (!g_compositor) return nullptr;

    /* Collect surfaces, then iterate in reverse (z-order: last committed = top). */
    ElsewhereSurface* surfaces[64];
    int count = 0;

    ElsewhereSurface *s, *s_next;
    wl_list_for_each_safe(s, s_next, &g_compositor->surface_list, link) {
        if (count < 64) {
            surfaces[count++] = s;
        }
    }

    for (int i = count - 1; i >= 0; i--) {
        ElsewhereSurface* surf = surfaces[i];
        int sx = wl_fixed_to_int(surf->current_x);
        int sy = wl_fixed_to_int(surf->current_y);
        if (x >= wl_fixed_from_int(sx) && x < wl_fixed_from_int(sx + surf->width) &&
            y >= wl_fixed_from_int(sy) && y < wl_fixed_from_int(sy + surf->height)) {
            return surf->resource;
        }
    }
    return nullptr;
}

/* Check if pointer position changed surface and send enter/leave. */
static void pointer_check_focus(uint32_t serial) {
    if (!g_seat) return;

    struct wl_resource* new_surface =
        g_seat->grab_surface_resource ? g_seat->grab_surface_resource
                                      : pointer_hit_test(pointer_x, pointer_y);

    if (new_surface != g_seat->pointer_surface_resource) {
        /* Leave old surface. */
        if (g_seat->pointer_surface_resource) {
            SeatPointerClient *pk, *pk_next;
            wl_list_for_each_safe(pk, pk_next, &g_seat->pointer_clients, seat_link) {
                if (pk->resource) {
                    wl_pointer_send_leave(pk->resource, serial,
                                          g_seat->pointer_surface_resource);
                }
            }
        }

        g_seat->pointer_surface_resource = new_surface;

        /* Enter new surface. */
        if (new_surface) {
            ElsewhereSurface* ms =
                compositor_surface_from_resource(new_surface);
            if (ms) {
                /* Compute position relative to the surface. */
                wl_fixed_t rx = pointer_x - wl_fixed_from_int(ms->current_x);
                wl_fixed_t ry = pointer_y - wl_fixed_from_int(ms->current_y);
                SeatPointerClient *pk, *pk_next;
                wl_list_for_each_safe(pk, pk_next, &g_seat->pointer_clients, seat_link) {
                    if (pk->resource) {
                        wl_pointer_send_enter(pk->resource, serial,
                                              new_surface, rx, ry);
                    }
                }
            }
        }
    }
}

void input_process(void) {}

/* Window close state for Wayland client (xdg_toplevel.close). */
static bool g_window_closed = false;

/* Process Wayland client events (dispatch pending events, check close flag).
 * Returns 0 on success, -1 if the window was closed by the user. */
int input_process_wayland_client(struct ElsewhereDisplay* m_display) {
    if (g_window_closed) {
        g_window_closed = false;
        return -1;
    }
    if (m_display && m_display->wl_client_display) {
        wl_display_dispatch_pending(m_display->wl_client_display);
        wl_display_flush(m_display->wl_client_display);
    }
    return 0;
}

/* Called from the toplevel.close listener in display.cpp. */
void input_handle_window_close(void) {
    g_window_closed = true;
}

/* Wayland seat input callbacks — update live camera movement state.
 * Called from the Wayland client display event handlers in display.cpp. */
void input_wayland_focus(bool focused) {
    live_focused = focused;
    live_movement = {};
}

void input_wayland_key(uint32_t key, bool pressed) {
    if (!live_focused || input_mode_get() != InputMode::World) return;
    switch (key) {
        case KEY_W: live_movement.w = pressed; break;
        case KEY_A: live_movement.a = pressed; break;
        case KEY_S: live_movement.s = pressed; break;
        case KEY_D: live_movement.d = pressed; break;
        default: break;
    }
}

void input_wayland_pointer_motion(double dx, double dy) {
    if (!live_focused || input_mode_get() != InputMode::World) return;
    live_movement.mouseX += dx;
    live_movement.mouseY += dy;
}

/* ---------- Input script execution (P1-T06-E) ---------- */

struct InputScript* input_script_init(const char* filename) {
    auto* s = new InputScript();
    s->fp = fopen(filename, "r");
    if (!s->fp) {
        fprintf(stderr, "Failed to open input script: %s\n", filename);
        delete s;
        return nullptr;
    }
    return s;
}

static void apply_movement(ElsewhereDisplay* display, double delta_ms,
                           MovementState* ms) {
    if (!display || !ms || delta_ms <= 0) return;
    if (display->flat_mode) return;

    auto* cam = &display->camera;
    float yaw = cam->yaw;
    float pitch = cam->pitch;

    float forwardX = -std::sin(yaw) * std::cos(pitch);
    float forwardY = std::sin(pitch);
    float forwardZ = -std::cos(yaw) * std::cos(pitch);

    float rightX =  std::cos(yaw);
    float rightZ = -std::sin(yaw);

    float dx = 0, dy = 0, dz = 0;
    float speed = static_cast<float>(MOVEMENT_SPEED * delta_ms);

    if (ms->w) { dx += forwardX * speed; dy += forwardY * speed; dz += forwardZ * speed; }
    if (ms->s) { dx -= forwardX * speed; dy -= forwardY * speed; dz -= forwardZ * speed; }
    if (ms->a) { dx -= rightX * speed;   dz -= rightZ * speed; }
    if (ms->d) { dx += rightX * speed;   dz += rightZ * speed; }
    if (ms->up)    dy += speed;
    if (ms->down)  dy -= speed;

    cam->x += dx;
    cam->y += dy;
    cam->z += dz;

    /* P4-T01: AABB collision — clamp camera inside room bounds (room mode only). */
    if (display->room_mode)
        room_apply_collision(&cam->x, &cam->y, &cam->z);
}

static void apply_mouse_look(ElsewhereDisplay* display,
                             MovementState* ms) {
    if (!display || !ms || display->flat_mode) return;
    /* Zero delta means no motion — no need for a button gate. */

    auto* cam = &display->camera;
    cam->yaw   -= static_cast<float>(ms->mouseX * MOUSE_SENSITIVITY);
    cam->pitch -= static_cast<float>(ms->mouseY * MOUSE_SENSITIVITY);

    /* Clamp pitch to ±89° to avoid flipping. */
    const float MAX_PITCH = 89.0f * 3.14159265f / 180.0f;
    if (cam->pitch > MAX_PITCH) cam->pitch = MAX_PITCH;
    if (cam->pitch < -MAX_PITCH) cam->pitch = -MAX_PITCH;

    ms->mouseX = 0;
    ms->mouseY = 0;
}

void input_wayland_apply_movement(ElsewhereDisplay* display, double delta_ms) {
    if (!live_focused || input_mode_get() != InputMode::World) {
        live_movement = {};
        return;
    }
    apply_movement(display, delta_ms, &live_movement);
    apply_mouse_look(display, &live_movement);
}

/* ── P5-T02: keyboard focus cycling ──────────────────────────────────────── */

/* Forward declaration — xdg_shell_cycle_focus is defined in xdg-shell.cpp. */

/* Execute the next command from the script. Returns 0 if more commands
   remain, 1 if quit, -1 on EOF. */
int input_script_step(struct InputScript* s,
                      struct ElsewhereSeat* seat,
                      struct ElsewhereCompositor* comp,
                      struct ElsewhereDisplay* display) {
    if (!s || s->done) return -1;
    if (!seat || !comp) return -1;
    if (s->remaining_wait_ms > 0) return 0;

    char line[256];
    if (!fgets(line, sizeof(line), s->fp)) {
        fclose(s->fp); s->fp = nullptr; s->done = 1;
        return -1;
    }

    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
        line[--len] = '\0';
    if (len == 0) return input_script_step(s, seat, comp, display);
    if (line[0] == '#') return input_script_step(s, seat, comp, display);

    if (strncmp(line, "wait ", 5) == 0) {
        s->remaining_wait_ms = strtol(line + 5, nullptr, 10);
        return 0;
    } else if (strncmp(line, "mode ", 5) == 0) {
        if (strcmp(line + 5, "world") == 0) {
            input_mode_set(InputMode::World);
            /* P3-T04: restore flat mode and toplevel size when returning to world. */
            if (display) {
                display->flat_mode = display->flat_mode_prev;
            }
            if (comp && comp->focused_surface_resource) {
                auto* surface_data =
                    compositor_surface_from_resource(comp->focused_surface_resource);
                if (surface_data && surface_data->xdg_surface) {
                    xdg_surface_restore_toplevel_size(surface_data->xdg_surface);
                }
            }
        } else if (strcmp(line + 5, "app") == 0) {
            input_mode_set(InputMode::Application);
            /* P3-T04: switch to flat/fullscreen rendering.
             * Save the current flat_mode so it can be restored, then set it
             * to false so render_application_fullscreen() is invoked instead of
             * the generic surface-list path. */
            if (display) {
                display->flat_mode_prev = display->flat_mode;
                display->flat_mode = false;
            }
            /* Save the toplevel's current size and resize to window dimensions. */
            if (comp && comp->focused_surface_resource && display) {
                auto* surface_data =
                    compositor_surface_from_resource(comp->focused_surface_resource);
                if (surface_data && surface_data->xdg_surface) {
                    xdg_surface_save_toplevel_size(surface_data->xdg_surface);
                    xdg_shell_send_configure_resize(
                        surface_data->xdg_surface,
                        display->window_width, display->window_height);
                }
            }
        }
        return 0;
    } else if (strncmp(line, "key ", 4) == 0) {
        char code_str[16] = {}, action[16] = {};
        sscanf(line + 4, "%15s %15s", code_str, action);
        int keycode = strtol(code_str, nullptr, 10);
        uint32_t state = (strcmp(action, "press") == 0) ?
            WL_KEYBOARD_KEY_STATE_PRESSED : WL_KEYBOARD_KEY_STATE_RELEASED;
        uint32_t serial = ++seat->serial;

        /* P3-T01: In World mode, key events are consumed (drive camera only).
         * In Application mode, events go to the focused surface. */
        if (g_input_mode == InputMode::Application) {
            if (seat->xkbstate)
                xkb_state_update_key(seat->xkbstate, keycode + 8,
                    state == WL_KEYBOARD_KEY_STATE_PRESSED ? XKB_KEY_DOWN : XKB_KEY_UP);
            SeatKeyboardClient *kc, *kc_next;
            wl_list_for_each_safe(kc, kc_next, &seat->keyboard_clients, seat_link)
                if (kc->resource)
                    wl_keyboard_send_key(kc->resource, serial, 0, keycode, state);
            send_keymap_modifiers(serial);

            /* P3-T02: track pressed keys for clean exit. */
            if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
                seat->pressed_keys.push_back(keycode);
            } else {
                auto it = std::find(seat->pressed_keys.begin(), seat->pressed_keys.end(), keycode);
                if (it != seat->pressed_keys.end())
                    seat->pressed_keys.erase(it);
            }

            /* P3-T02: world-key shortcut — exit Application mode and switch to World. */
            if (state == WL_KEYBOARD_KEY_STATE_PRESSED && keycode == g_world_key) {
                exit_application_mode();
                input_mode_set(InputMode::World);
                return 0;
            }

            /* P4-T02: teleport key — move camera to stored viewpoint. */
            if (state == WL_KEYBOARD_KEY_STATE_PRESSED &&
                keycode == g_teleport_key && display) {
                input_teleport(display);
                return 0;
            }
        }

        /* P3-T05: In World mode, Enter (key 28) on a targeted panel
         * enters Application mode. */
        if (g_input_mode == InputMode::World &&
            display && display->panel_targeted) {
            if (state == WL_KEYBOARD_KEY_STATE_PRESSED && keycode == 28) {
                input_mode_set(InputMode::Application);
                return 0;
            }
        }

        /* P4-T02: teleport key — move camera to stored viewpoint (works in
         * both World and Application mode; in App mode it already returned
         * above, this handles the World mode path). */
        if (state == WL_KEYBOARD_KEY_STATE_PRESSED &&
            keycode == g_teleport_key && display) {
            input_teleport(display);
            return 0;
        }

        /* P2-T06 / P3-T01: WASD + arrow keys always drive camera movement
         * regardless of input mode. */
        if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
            switch (keycode) {
            case KEY_W:      s->movement.w = true;     break;
            case KEY_A:      s->movement.a = true;     break;
            case KEY_S:      s->movement.s = true;     break;
            case KEY_D:      s->movement.d = true;     break;
            case KEY_UP:     s->movement.up = true;    break;
            case KEY_DOWN:   s->movement.down = true;  break;
            case KEY_LEFT:   s->movement.left = true;  break;
            case KEY_RIGHT:  s->movement.right = true; break;
            default: break;
            }
        } else {
            switch (keycode) {
            case KEY_W:      s->movement.w = false;     break;
            case KEY_A:      s->movement.a = false;     break;
            case KEY_S:      s->movement.s = false;     break;
            case KEY_D:      s->movement.d = false;     break;
            case KEY_UP:     s->movement.up = false;    break;
            case KEY_DOWN:   s->movement.down = false;  break;
            case KEY_LEFT:   s->movement.left = false;  break;
            case KEY_RIGHT:  s->movement.right = false; break;
            default: break;
            }
        }
    } else if (strcmp(line, "tab") == 0) {
        /* P5-T02: tab key — cycle focus forward in World mode, pass
         * through to client in Application mode. */
        if (g_input_mode == InputMode::World) {
            xdg_shell_cycle_focus(seat, comp, true);
        }
    } else if (strcmp(line, "shift_tab") == 0) {
        /* P5-T02: shift+tab — cycle focus backward in World mode. */
        if (g_input_mode == InputMode::World) {
            xdg_shell_cycle_focus(seat, comp, false);
        }
    } else if (strncmp(line, "motion ", 7) == 0) {
        int x = 0, y = 0;
        sscanf(line + 7, "%d %d", &x, &y);
        if (s->movement.right_button_pressed) {
            /* P2-T06: mouse look — accumulate delta. */
            s->movement.mouseX += static_cast<double>(x);
            s->movement.mouseY += static_cast<double>(y);
        } else if (g_input_mode == InputMode::Application) {
            /* P3-T01: pointer motion only goes to clients in Application mode. */
            pointer_x = wl_fixed_from_int(x);
            pointer_y = wl_fixed_from_int(y);
            uint32_t serial = ++seat->serial;
            pointer_check_focus(serial);
            send_pointer_motion(0);
            send_pointer_axis_done(0);
        }
    } else if (strncmp(line, "button ", 7) == 0) {
        char code_str[16] = {}, action[16] = {};
        sscanf(line + 7, "%15s %15s", code_str, action);
        uint32_t button = (uint32_t)strtol(code_str, nullptr, 10);
        uint32_t button_state = (strcmp(action, "press") == 0) ?
            WL_POINTER_BUTTON_STATE_PRESSED : WL_POINTER_BUTTON_STATE_RELEASED;
        if (button_state == WL_POINTER_BUTTON_STATE_PRESSED)
            seat->grab_surface_resource = seat->pointer_surface_resource;
        else
            seat->grab_surface_resource = nullptr;
        /* P2-T06: track right button state for mouse look. */
        if (button == BTN_RIGHT)
            s->movement.right_button_pressed = (button_state == WL_POINTER_BUTTON_STATE_PRESSED);
        /* P3-T01: pointer button events only go to clients in Application mode. */
        if (g_input_mode == InputMode::Application) {
            uint32_t serial = ++seat->serial;
            send_pointer_button(serial, 0, button, button_state);
        }
    } else if (strcmp(line, "focus gained") == 0) {
        /* P3-T03: re-enter application mode only if stored mode was Application. */
        if (g_seat && stored_mode_when_focus_lost == InputMode::Application
            && g_compositor && g_compositor->focused_surface_resource) {
            input_mode_set(InputMode::Application);
            seat_set_keyboard_focus(seat, g_compositor->focused_surface_resource,
                                    g_compositor);
        } else if (g_seat && g_compositor && g_compositor->focused_surface_resource) {
            /* Default: restore keyboard focus regardless of mode. */
            seat_set_keyboard_focus(seat, g_compositor->focused_surface_resource,
                                    g_compositor);
        }
    } else if (strcmp(line, "focus lost") == 0) {
        /* P3-T03: save current mode, then exit application mode and switch to World. */
        if (g_seat) {
            stored_mode_when_focus_lost = input_mode_get();
            exit_application_mode();
            input_mode_set(InputMode::World);
        } else if (seat) {
            /* Fallback: just clear keyboard focus. */
            seat_set_keyboard_focus(seat, nullptr, comp);
        }
    } else if (strcmp(line, "quit") == 0) {
        return 1;
    }
    return 0;
}

void input_script_destroy(struct InputScript* s) {
    if (!s) return;
    if (s->fp) fclose(s->fp);
    delete s;
}

void input_script_apply_movement(struct InputScript* s,
                                 struct ElsewhereDisplay* display,
                                 double delta_ms) {
    if (!s || !display) return;
    apply_movement(display, delta_ms, &s->movement);
    apply_mouse_look(display, &s->movement);
}


// Legacy policy owns its global seat; the reusable server has no global seat.
ElsewhereSeat* create_seat(wl_display* display) {
    stored_mode_when_focus_lost = InputMode::World;
    g_seat = seat_create(display);
    return g_seat;
}
void destroy_seat(ElsewhereSeat* seat) {
    if (g_seat == seat) g_seat = nullptr;
    seat_destroy(seat);
}
