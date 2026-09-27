#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <vector>
#include <algorithm>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <wayland-server-protocol.h>
#include <xkbcommon/xkbcommon.h>

#include "compositor.h"
#include "compositor-private.h"
#include "display.h"
#include "input.h"
#include "room.h"
#include "xdg-shell.h"

/* Evdev button codes (from linux/input-event-codes.h). */
static constexpr uint32_t BTN_LEFT   = 0x110;
static constexpr uint32_t BTN_RIGHT  = 0x111;
static constexpr uint32_t BTN_MIDDLE = 0x112;

/* memfd_create may not be declared without _GNU_SOURCE. */
extern "C" int memfd_create(const char *name, unsigned int flags);

/* Current pointer state */
static wl_fixed_t pointer_x = wl_fixed_from_int(300);
static wl_fixed_t pointer_y = wl_fixed_from_int(200);

/* ---------- Input mode (P3-T01) ---------- */
static InputMode g_input_mode = InputMode::World;

/* P3-T02: evdev keycode for switching to world mode (default: F12 = 88). */
static int g_world_key = 88;

/* P4-T02: evdev keycode for teleport (default: T = 20). */
static int g_teleport_key = 0;

void input_mode_set(InputMode mode) {
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

/* Global pointer to seat for input forwarding */
struct MansionSeat* g_seat = nullptr;
/* Global pointer to compositor for pointer hit testing. */
struct MansionCompositor* g_compositor = nullptr;

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

struct MansionSeat {
    struct wl_global* global;
    struct wl_list keyboard_clients;   /* SeatKeyboardClient */
    struct wl_list pointer_clients;    /* SeatPointerClient */

    xkb_keymap* keymap;
    xkb_state*  xkbstate;

    /* Keyboard focus (P1-T06-C). */
    struct wl_resource* focused_surface_resource;

    /* Pointer focus and grab (P1-T06-D). */
    struct wl_resource* pointer_surface_resource; /* surface pointer is over */
    struct wl_resource* grab_surface_resource;    /* surface with pointer grab */
    uint32_t serial;                              /* shared serial for all events */

    /* P3-T02: track pressed keycodes for clean exit from Application mode. */
    std::vector<uint32_t> pressed_keys;

    /* P3-T03: saved mode when host focus is lost (re-apply on focus gained). */
    InputMode stored_mode_when_focus_lost = InputMode::World;
};

int input_init(void) {
    /* X11 input is handled via input_process_x11() in the main loop
     * (windowed mode) or via --input-script (headless / tests). */
    return 0;
}

void input_destroy(void) {
    /* Nothing to clean up — X11 is managed by the display. */
}

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
    MansionSurface* surfaces[64];
    int count = 0;

    MansionSurface *s, *s_next;
    wl_list_for_each_safe(s, s_next, &g_compositor->surface_list, link) {
        if (count < 64) {
            surfaces[count++] = s;
        }
    }

    for (int i = count - 1; i >= 0; i--) {
        MansionSurface* surf = surfaces[i];
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
            MansionSurface* ms =
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

void input_process(void) {
    /* X11 events are processed via input_process_x11() in windowed mode.
     * Evdev is no longer used. */
}

/* ---------- X11 event processing (P1-T07) ---------- */

static Atom wm_delete_window_atom = None;

int input_process_x11(struct MansionDisplay* m_display) {
    if (!m_display || !m_display->x_display) return 0;

    Display* xdpy = m_display->x_display;

    /* Lazily create the WM_DELETE_WINDOW atom. */
    if (!wm_delete_window_atom) {
        wm_delete_window_atom =
            XInternAtom(xdpy, "WM_DELETE_WINDOW", False);
    }

    XEvent ev;
    while (XPending(xdpy) > 0) {
        XNextEvent(xdpy, &ev);

        switch (ev.type) {
        /* ---- Key events ---- */
        case KeyPress:
        case KeyRelease: {
            uint32_t x11_keycode = ev.xkey.keycode;   /* X11 keycode (≥ 8) */
            uint32_t linux_keycode = x11_keycode - 8; /* → evdev / Linux keycode */
            uint32_t time_msec   = ev.xkey.time;
            uint32_t state       = (ev.type == KeyPress) ?
                WL_KEYBOARD_KEY_STATE_PRESSED :
                WL_KEYBOARD_KEY_STATE_RELEASED;

            /* Update xkb state (xkbcommon uses X11 keycode range). */
            if (g_seat && g_seat->xkbstate) {
                xkb_state_update_key(g_seat->xkbstate, x11_keycode,
                    state == WL_KEYBOARD_KEY_STATE_PRESSED ?
                        XKB_KEY_DOWN : XKB_KEY_UP);
            }

            uint32_t serial = ++g_seat->serial;

            /* Send key event to all keyboard clients. */
            {
                SeatKeyboardClient *kc, *kc_next;
                wl_list_for_each_safe(kc, kc_next,
                    &g_seat->keyboard_clients, seat_link) {
                    if (kc->resource)
                        wl_keyboard_send_key(kc->resource,
                            serial, time_msec, linux_keycode, state);
                }
            }

            /* Send updated modifier state. */
            send_keymap_modifiers(serial);
            break;
        }

        /* ---- Pointer button events ---- */
        case ButtonPress:
        case ButtonRelease: {
            uint32_t button = 0;
            switch (ev.xbutton.button) {
            case 1: button = BTN_LEFT;   break;
            case 2: button = BTN_MIDDLE; break;
            case 3: button = BTN_RIGHT;  break;
            /* Buttons 4/5 = wheel — handled below via axis. */
            default: break;
            }

            uint32_t serial;
            if (button) {
                uint32_t button_state = (ev.type == ButtonPress) ?
                    WL_POINTER_BUTTON_STATE_PRESSED :
                    WL_POINTER_BUTTON_STATE_RELEASED;

                if (button_state == WL_POINTER_BUTTON_STATE_PRESSED)
                    g_seat->grab_surface_resource = g_seat->pointer_surface_resource;
                else
                    g_seat->grab_surface_resource = nullptr;

                serial = ++g_seat->serial;
                send_pointer_button(serial, ev.xbutton.time,
                                    button, button_state);
            } else {
                /* Wheel: buttons 4 (up) and 5 (down). */
                uint32_t axis  = (ev.xbutton.button == 4) ?
                    static_cast<uint32_t>(WL_POINTER_AXIS_VERTICAL_SCROLL) :
                    static_cast<uint32_t>(WL_POINTER_AXIS_VERTICAL_SCROLL);
                wl_fixed_t value = (ev.xbutton.button == 4) ?
                    wl_fixed_from_int(10) : wl_fixed_from_int(-10);
                serial = ++g_seat->serial;
                send_pointer_axis(ev.xbutton.time,
                    static_cast<enum wl_pointer_axis>(axis), value);
            }
            break;
        }

        /* ---- Pointer motion events ---- */
        case MotionNotify: {
            pointer_x = wl_fixed_from_int(ev.xmotion.x);
            pointer_y = wl_fixed_from_int(ev.xmotion.y);

            uint32_t serial = ++g_seat->serial;
            pointer_check_focus(serial);
            send_pointer_motion(ev.xmotion.time);
            send_pointer_axis_done(ev.xmotion.time);
            break;
        }

        /* ---- Window resize ---- */
        case ConfigureNotify: {
            int new_w = ev.xconfigure.width;
            int new_h = ev.xconfigure.height;
            if (new_w > 0 && new_h > 0 &&
                (new_w != m_display->window_width ||
                 new_h != m_display->window_height)) {
                m_display->window_width  = new_w;
                m_display->window_height = new_h;
                display_resize(m_display);
            }
            break;
        }

        /* ---- WM_DELETE_WINDOW (window close) ---- */
        case ClientMessage: {
            if (ev.xclient.message_type == wm_delete_window_atom) {
                return -1;
            }
            break;
        }

        /* ---- Focus changes (informational) ---- */
        case FocusOut:
            /* P3-T03: save current mode, then exit application mode and switch to World. */
            if (g_seat) {
                g_seat->stored_mode_when_focus_lost = input_mode_get();
                exit_application_mode();
                input_mode_set(InputMode::World);
            }
            break;
        case FocusIn:
            /* P3-T03: re-enter application mode if stored mode was Application. */
            if (g_seat && g_seat->stored_mode_when_focus_lost == InputMode::Application
                && g_compositor && g_compositor->focused_surface_resource) {
                input_mode_set(InputMode::Application);
                seat_set_keyboard_focus(g_seat,
                    g_compositor->focused_surface_resource, g_compositor);
            } else if (g_seat && g_compositor &&
                       g_compositor->focused_surface_resource) {
                seat_set_keyboard_focus(g_seat,
                    g_compositor->focused_surface_resource, g_compositor);
            }
            break;

        default:
            break;
        }
    }

    return 0;
}

/* ---------- Input script execution (P1-T06-E) ---------- */

/* Movement key codes (evdev / linux/input-event-codes.h). */
static constexpr unsigned KEY_W       = 17;
static constexpr unsigned KEY_A       = 30;
static constexpr unsigned KEY_S       = 31;
static constexpr unsigned KEY_D       = 32;
static constexpr unsigned KEY_UP      = 103;
static constexpr unsigned KEY_DOWN    = 108;
static constexpr unsigned KEY_LEFT    = 105;
static constexpr unsigned KEY_RIGHT   = 106;

/* Movement speed: 0.01 units per millisecond.
 * Moving from Z=10 to Z=5 takes 500 ms. */
static constexpr double MOVEMENT_SPEED = 0.01;

/* Mouse-look sensitivity in radians per pixel. */
static constexpr double MOUSE_SENSITIVITY = 0.002;

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

static void apply_movement(MansionDisplay* display, double delta_ms,
                           MovementState* ms) {
    if (!display || !ms || !display->renderer || delta_ms <= 0) return;
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

static void apply_mouse_look(MansionDisplay* display,
                             MovementState* ms) {
    if (!display || !ms || !display->renderer || display->flat_mode) return;
    if (!ms->right_button_pressed) return;

    auto* cam = &display->camera;
    cam->yaw   += static_cast<float>(ms->mouseX * MOUSE_SENSITIVITY);
    cam->pitch += static_cast<float>(ms->mouseY * MOUSE_SENSITIVITY);

    /* Clamp pitch to ±89° to avoid flipping. */
    const float MAX_PITCH = 89.0f * 3.14159265f / 180.0f;
    if (cam->pitch > MAX_PITCH) cam->pitch = MAX_PITCH;
    if (cam->pitch < -MAX_PITCH) cam->pitch = -MAX_PITCH;

    ms->mouseX = 0;
    ms->mouseY = 0;
}

/* Execute the next command from the script. Returns 0 if more commands
   remain, 1 if quit, -1 on EOF. */
int input_script_step(struct InputScript* s,
                      struct MansionSeat* seat,
                      struct MansionCompositor* comp,
                      struct MansionDisplay* display) {
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
        if (g_seat && g_seat->stored_mode_when_focus_lost == InputMode::Application
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
            g_seat->stored_mode_when_focus_lost = input_mode_get();
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
                                 struct MansionDisplay* display,
                                 double delta_ms) {
    if (!s || !display) return;
    apply_movement(display, delta_ms, &s->movement);
    apply_mouse_look(display, &s->movement);
}


/* ---------- SeatKeyboardClient destroy listener ---------- */

static void keyboard_client_destroy(struct wl_listener* listener, void* data) {
    (void)data;
    SeatKeyboardClient* kbc = wl_container_of(listener, kbc, destroy_listener);
    /* Do NOT remove from the list or delete here — the Wayland library
     * may still hold references during teardown and we crash.
     * Instead we just null the resource; destroy_seat will clean up. */
    kbc->resource = nullptr;
}

/* ---------- SeatPointerClient destroy listener ---------- */

static void pointer_client_destroy(struct wl_listener* listener, void* data) {
    (void)data;
    SeatPointerClient* pkc = wl_container_of(listener, pkc, destroy_listener);
    /* Do NOT remove from the list or delete here — the Wayland library
     * may still hold references during teardown and we crash.
     * Instead we just null the resource; destroy_seat will clean up. */
    pkc->resource = nullptr;
}

/* ---------- Pointer implementation ---------- */

static void pointer_set_cursor(struct wl_client* client, struct wl_resource* resource,
                                uint32_t serial, struct wl_resource* surface,
                                int32_t x, int32_t y) {
    (void)client; (void)resource; (void)serial; (void)surface; (void)x; (void)y;
}

static const struct wl_pointer_interface pointer_impl = {
    pointer_set_cursor,
    nullptr, /* release (v4, optional) */
};

/* ---------- Keyboard implementation ---------- */

static const struct wl_keyboard_interface keyboard_impl = {
    nullptr, /* release (v3, optional) */
};

/* ---------- Seat implementations ---------- */

static void seat_get_pointer(struct wl_client* client, struct wl_resource* seat_resource,
                               uint32_t id) {
    auto* seat = static_cast<MansionSeat*>(wl_resource_get_user_data(seat_resource));

    auto* kpc = new SeatPointerClient;
    memset(kpc, 0, sizeof(*kpc));

    auto* resource = wl_resource_create(client, &wl_pointer_interface, 4, id);
    if (!resource) {
        delete kpc;
        wl_client_post_no_memory(client);
        return;
    }

    wl_resource_set_implementation(resource, &pointer_impl, seat, nullptr);
    kpc->resource = resource;

    // Listen for client destruction of this pointer resource
    wl_listener* destroy_listener = &kpc->destroy_listener;
    destroy_listener->notify = pointer_client_destroy;
    wl_signal_add(&resource->destroy_signal, destroy_listener);

    // Track in seat's client list using a separate link
    wl_list_insert(&seat->pointer_clients, &kpc->seat_link);

    /* Don't send a synthetic enter here; proper enter/leave is handled
       by pointer_check_focus() when the pointer position changes. */

    // Send capabilities (done at seat level, not per-client)
}

static void seat_get_keyboard(struct wl_client* client, struct wl_resource* seat_resource,
                               uint32_t id) {
    auto* seat = static_cast<MansionSeat*>(wl_resource_get_user_data(seat_resource));

    auto* kbc = new SeatKeyboardClient;
    memset(kbc, 0, sizeof(*kbc));

    auto* resource = wl_resource_create(client, &wl_keyboard_interface, 7, id);
    if (!resource) {
        delete kbc;
        wl_client_post_no_memory(client);
        return;
    }

    wl_resource_set_implementation(resource, &keyboard_impl, seat, nullptr);
    kbc->resource = resource;

    // Listen for client destruction of this keyboard resource
    wl_listener* destroy_listener = &kbc->destroy_listener;
    destroy_listener->notify = keyboard_client_destroy;
    wl_signal_add(&resource->destroy_signal, destroy_listener);

    // Track in seat's client list using a separate link
    wl_list_insert(&seat->keyboard_clients, &kbc->seat_link);

    // Send keymap
    if (seat->keymap) {
        const char* keymap_string = xkb_keymap_get_as_string(
            seat->keymap, XKB_KEYMAP_FORMAT_TEXT_V1);

        if (keymap_string) {
            size_t keymap_len = strlen(keymap_string);
            int fd = memfd_create("xkb-keymap", 0);
            if (fd >= 0) {
                write(fd, keymap_string, keymap_len);
                lseek(fd, 0, SEEK_SET);
                wl_keyboard_send_keymap(resource, WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1,
                                        fd, (uint32_t)keymap_len);
                close(fd);
            }
        }
    }

    // Send repeat_info (rate = 25 keys/sec, delay = 500 ms)
    wl_keyboard_send_repeat_info(resource, 25, 500);
}

static void seat_get_touch(struct wl_client* client, struct wl_resource* seat_resource,
                             uint32_t id) {
    (void)client; (void)seat_resource; (void)id;
    wl_resource_post_error(seat_resource, WL_SEAT_ERROR_MISSING_CAPABILITY,
                           "seat does not have touch capability");
}

static void seat_release(struct wl_client* client, struct wl_resource* seat_resource) {
    (void)client;
    wl_resource_destroy(seat_resource);
}

static const struct wl_seat_interface seat_impl = {
    seat_get_pointer,
    seat_get_keyboard,
    seat_get_touch,
    seat_release,
};

static void seat_bind(struct wl_client* client, void* data, uint32_t version, uint32_t id) {
    auto* seat = static_cast<MansionSeat*>(data);
    (void)version;

    auto* resource = wl_resource_create(client, &wl_seat_interface, 4, id);
    if (!resource) {
        wl_client_post_no_memory(client);
        return;
    }

    wl_resource_set_implementation(resource, &seat_impl, seat, nullptr);

    // Send seat name
    wl_seat_send_name(resource, "default-seat");

    // Send capabilities (pointer + keyboard)
    wl_seat_send_capabilities(resource, WL_SEAT_CAPABILITY_POINTER | WL_SEAT_CAPABILITY_KEYBOARD);
}

struct MansionSeat* create_seat(struct wl_display* display) {
    auto* seat = new MansionSeat();
    wl_list_init(&seat->keyboard_clients);
    wl_list_init(&seat->pointer_clients);
    seat->serial = 1;

    seat->global = wl_global_create(display, &wl_seat_interface, 4, seat, seat_bind);
    if (!seat->global) {
        delete seat;
        return nullptr;
    }

    // Create xkb keymap. xkb_keymap_new_from_names requires xkbcommon >= 1.0;
    // fall back to a hardcoded keymap on older versions.
    xkb_context* ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (ctx) {
        seat->keymap = xkb_keymap_new_from_string(
            ctx,
            "xkb_keymap {\n"
            "    xkb_keycodes  { include \"evdev+aliases(qwerty)\" };\n"
            "    xkb_types     { include \"complete\" };\n"
            "    xkb_compat    { include \"complete\" };\n"
            "    xkb_symbols   { include \"pc+us+inet(evdev)\" };\n"
            "    xkb_geometry  { include \"pc(pc105)\" };\n"
            "};\n",
            XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
        if (seat->keymap) {
            seat->xkbstate = xkb_state_new(seat->keymap);
        }
        xkb_context_unref(ctx);
    }

    if (!seat->keymap) {
        fprintf(stderr, "Warning: could not create xkb keymap\n");
        // Use hardcoded fallback
        seat->keymap = xkb_keymap_new_from_string(
            nullptr,
            "xkb_keymap {\n"
            "    xkb_keycodes  { include \"evdev+aliases(qwerty)\" };\n"
            "    xkb_types     { include \"complete\" };\n"
            "    xkb_compat    { include \"complete\" };\n"
            "    xkb_symbols   { include \"pc+us+inet(evdev)\" };\n"
            "    xkb_geometry  { include \"pc(pc105)\" };\n"
            "};\n",
            XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
        if (seat->keymap) {
            seat->xkbstate = xkb_state_new(seat->keymap);
        }
    }

    // Register as global seat for input forwarding
    g_seat = seat;

    return seat;
}

void destroy_seat(struct MansionSeat* seat) {
    if (!seat) return;

    // Unregister global seat
    if (g_seat == seat) {
        g_seat = nullptr;
    }

    // Clear focus before destroying resources to avoid stale references.
    seat->grab_surface_resource = nullptr;
    seat->pointer_surface_resource = nullptr;
    seat->focused_surface_resource = nullptr;

    // Destroy all keyboard clients.
    // If a client was destroyed earlier (Wayland library), its resource is
    // nullptr — we still delete the struct.  We do NOT call wl_resource_destroy
    // here; wl_display_destroy handles it.
    {
        SeatKeyboardClient *kc, *kc_next;
        wl_list_for_each_safe(kc, kc_next, &seat->keyboard_clients, seat_link) {
            wl_list_remove(&kc->seat_link);
            delete kc;
        }
    }

    // Destroy all pointer clients.
    {
        SeatPointerClient *pk, *pk_next;
        wl_list_for_each_safe(pk, pk_next, &seat->pointer_clients, seat_link) {
            wl_list_remove(&pk->seat_link);
            delete pk;
        }
    }

    wl_global_destroy(seat->global);

    // Clean up xkb state and keymap
    if (seat->xkbstate) {
        xkb_state_unref(seat->xkbstate);
    }
    if (seat->keymap) {
        xkb_keymap_unref(seat->keymap);
    }
    seat->xkbstate = nullptr;
    seat->keymap = nullptr;
    delete seat;
}

/* ---------- Keyboard focus (P1-T06-C) ---------- */

void seat_set_keyboard_focus(struct MansionSeat* seat,
                              struct wl_resource* surface,
                              MansionCompositor* comp) {
    comp->keyboard_focus_serial++;

    /* Send leave to the old focused surface's keyboard clients.
     * Only send to clients whose resource belongs to the same client
     * as the old focused surface — libwayland validates this and
     * rejects cross-client surface pointers as a protocol error. */
    if (comp->focused_surface_resource) {
        SeatKeyboardClient *kc, *kc_next;
        struct wl_client *old_client = wl_resource_get_client(
            comp->focused_surface_resource);
        wl_list_for_each_safe(kc, kc_next, &seat->keyboard_clients, seat_link) {
            if (kc->resource &&
                wl_resource_get_client(kc->resource) == old_client) {
                wl_keyboard_send_leave(kc->resource,
                                       comp->keyboard_focus_serial,
                                       comp->focused_surface_resource);
            }
        }
    }
    comp->focused_surface_resource = surface;
    seat->focused_surface_resource = surface;

    /* Send enter to the new surface's keyboard clients.
     * Only send to clients whose resource belongs to the same client
     * as the new surface. */
    if (surface) {
        struct wl_array keys;
        wl_array_init(&keys);
        SeatKeyboardClient *kc, *kc_next;
        struct wl_client *new_client = wl_resource_get_client(surface);
        wl_list_for_each_safe(kc, kc_next, &seat->keyboard_clients, seat_link) {
            if (kc->resource &&
                wl_resource_get_client(kc->resource) == new_client) {
                wl_keyboard_send_enter(kc->resource,
                                       comp->keyboard_focus_serial,
                                       surface, &keys);
            }
        }
        wl_array_release(&keys);
    }
}

void compositor_set_seat(struct MansionCompositor* compositor,
                          struct MansionSeat* seat) {
    if (!compositor) return;
    compositor->seat = seat;
}
