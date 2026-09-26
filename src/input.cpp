#include <cstring>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/input-event-codes.h>
#include <linux/uinput.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <poll.h>
#include <time.h>

#include <wayland-server-protocol.h>
#include <xkbcommon/xkbcommon.h>

#include "compositor.h"
#include "compositor-private.h"
#include "display.h"
#include "input.h"

/* Maximum number of input devices to track */
#define MAX_INPUT_DEVICES 16

/* Evdev device state */
static struct {
    int fd;
    bool is_keyboard;
    bool is_mouse;
    char name[64];
} input_devices[MAX_INPUT_DEVICES];
static int input_device_count = 0;

/* Current pointer state */
static wl_fixed_t pointer_x = wl_fixed_from_int(300);
static wl_fixed_t pointer_y = wl_fixed_from_int(200);

/* Global pointer to seat for input forwarding */
struct MansionSeat* g_seat = nullptr;
/* Global pointer to compositor for pointer hit testing. */
struct MansionCompositor* g_compositor = nullptr;

/* Per-client keyboard state */
struct SeatKeyboardClient {
    struct wl_resource* resource;
    struct wl_listener destroy_listener;
};

/* Per-client pointer state */
struct SeatPointerClient {
    struct wl_resource* resource;
    struct wl_listener destroy_listener;
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
};

/* Evdev device detection helpers */

static int evdev_open_device(const char* path) {
    int fd = open(path, O_RDONLY | O_NONBLOCK);
    if (fd < 0) return -1;

    // Check if this is a keyboard or mouse
    unsigned char key_bits[32];
    memset(key_bits, 0, sizeof(key_bits));
    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits) < 0) {
        close(fd);
        return -1;
    }

    bool is_keyboard = false;
    bool is_mouse = false;

    // Keyboard: has KEY_* codes (e.g., KEY_ENTER, KEY_A, etc.)
    if (key_bits[KEY_ENTER / 8] & (1 << (KEY_ENTER % 8))) {
        is_keyboard = true;
    }

    // Mouse: has REL_X or REL_Y in EV_REL
    unsigned char rel_bits[32];
    memset(rel_bits, 0, sizeof(rel_bits));
    if (ioctl(fd, EVIOCGBIT(EV_REL, sizeof(rel_bits)), rel_bits) >= 0) {
        if (rel_bits[REL_X / 8] & (1 << (REL_X % 8))) {
            is_mouse = true;
        }
    }

    if (!is_keyboard && !is_mouse) {
        close(fd);
        return -1;
    }

    // Get device name
    char name[64] = {0};
    ioctl(fd, EVIOCGNAME(sizeof(name)), name);

    // Store device info
    if (input_device_count < MAX_INPUT_DEVICES) {
        input_devices[input_device_count].fd = fd;
        input_devices[input_device_count].is_keyboard = is_keyboard;
        input_devices[input_device_count].is_mouse = is_mouse;
        strncpy(input_devices[input_device_count].name, name,
                sizeof(input_devices[input_device_count].name) - 1);
        input_devices[input_device_count].name[sizeof(input_devices[input_device_count].name) - 1] = '\0';
        input_device_count++;
        fprintf(stderr, "Input device: %s (%s) fd=%d\n", name,
                is_keyboard ? "keyboard" : "mouse", fd);
    } else {
        close(fd);
    }

    return fd;
}

int input_init(void) {
    DIR* dir = opendir("/dev/input");
    if (!dir) {
        fprintf(stderr, "Failed to open /dev/input\n");
        return -1;
    }

    input_device_count = 0;
    for (int i = 0; i < MAX_INPUT_DEVICES; i++) {
        input_devices[i].fd = -1;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        // Match event* files only
        if (entry->d_type != DT_CHR && entry->d_name[0] != 'e') continue;

        char path[256];
        snprintf(path, sizeof(path), "/dev/input/%s", entry->d_name);

        // Try direct open first (handles char devices properly)
        if (evdev_open_device(path) < 0) {
            // Fallback: try to open and check bits manually
            int fd = open(path, O_RDONLY | O_NONBLOCK);
            if (fd < 0) continue;

            unsigned char evbits[32];
            memset(evbits, 0, sizeof(evbits));
            if (ioctl(fd, EVIOCGBIT(0, sizeof(evbits)), evbits) < 0) {
                close(fd);
                continue;
            }

            // EV_KEY = 0x01, EV_REL = 0x02
            bool has_key = evbits[EV_KEY / 32] & (1 << (EV_KEY % 32));
            bool has_rel = evbits[EV_REL / 32] & (1 << (EV_REL % 32));

            if (!has_key && !has_rel) {
                close(fd);
                continue;
            }

            if (input_device_count < MAX_INPUT_DEVICES) {
                char name[64] = {0};
                ioctl(fd, EVIOCGNAME(sizeof(name)), name);
                input_devices[input_device_count].fd = fd;
                input_devices[input_device_count].is_keyboard = has_key;
                input_devices[input_device_count].is_mouse = has_rel;
                strncpy(input_devices[input_device_count].name, name, 63);
                input_devices[input_device_count].name[63] = '\0';
                input_device_count++;
                fprintf(stderr, "Input device: %s fd=%d\n", name, fd);
            } else {
                close(fd);
            }
        }
    }

    closedir(dir);

    if (input_device_count == 0) {
        fprintf(stderr, "No input devices found\n");
        return -1;
    }

    fprintf(stderr, "Initialized %d input devices\n", input_device_count);
    return 0;
}

/* Iterate over all pointer clients and send motion events. */
static void send_pointer_motion(uint32_t time_msec) {
    if (!g_seat) return;

    SeatPointerClient *pk, *pk_next;
    wl_list_for_each_safe(pk, pk_next, &g_seat->pointer_clients, destroy_listener.link) {
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
    wl_list_for_each_safe(pk, pk_next, &g_seat->pointer_clients, destroy_listener.link) {
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
    wl_list_for_each_safe(pk, pk_next, &g_seat->pointer_clients, destroy_listener.link) {
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
    wl_list_for_each_safe(kc, kc_next, &g_seat->keyboard_clients, destroy_listener.link) {
        if (kc->resource) {
            wl_keyboard_send_modifiers(kc->resource, serial,
                                       depressed, latched, locked, group);
        }
    }
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
            wl_list_for_each_safe(pk, pk_next, &g_seat->pointer_clients, destroy_listener.link) {
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
                wl_list_for_each_safe(pk, pk_next, &g_seat->pointer_clients, destroy_listener.link) {
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
    if (!g_seat) return;

    uint32_t time_msec = 0;
    uint32_t serial = ++g_seat->serial;

    for (int i = 0; i < input_device_count; i++) {
        struct input_event ev;
        ssize_t bytes;

        while ((bytes = read(input_devices[i].fd, &ev, sizeof(ev))) >= (ssize_t)sizeof(ev)) {
            time_msec = ev.time.tv_sec * 1000 + ev.time.tv_usec / 1000;

            // Handle keyboard events
            if (input_devices[i].is_keyboard && ev.type == EV_KEY &&
                ev.code >= KEY_MIN_INTERESTING) {
                uint32_t state = (ev.value == 1) ?
                    WL_KEYBOARD_KEY_STATE_PRESSED :
                    WL_KEYBOARD_KEY_STATE_RELEASED;

                // Update xkb state so modifiers are tracked correctly.
                // ev.code is a Linux keycode; xkb uses the same range.
                if (g_seat->xkbstate) {
                    xkb_state_update_key(g_seat->xkbstate,
                                         ev.code + 8,  // evdev -> xkb offset
                                         state == WL_KEYBOARD_KEY_STATE_PRESSED ?
                                             XKB_KEY_DOWN : XKB_KEY_UP);
                }

                SeatKeyboardClient *kc, *kc_next;
                wl_list_for_each_safe(kc, kc_next, &g_seat->keyboard_clients, destroy_listener.link) {
                    if (kc->resource) {
                        wl_keyboard_send_key(kc->resource, serial, time_msec, ev.code, state);
                    }
                }

                // Send updated modifier state
                send_keymap_modifiers(serial);
            }

            // Handle mouse events
            if (input_devices[i].is_mouse) {
                if (ev.type == EV_REL) {
                    if (ev.code == REL_X) {
                        pointer_x += wl_fixed_from_int(ev.value);
                    } else if (ev.code == REL_Y) {
                        pointer_y += wl_fixed_from_int(ev.value);
                    } else if (ev.code == REL_WHEEL) {
                        serial = ++g_seat->serial;
                        send_pointer_axis(time_msec, WL_POINTER_AXIS_VERTICAL_SCROLL,
                                          wl_fixed_from_int(ev.value * 10));
                    } else if (ev.code == REL_HWHEEL) {
                        serial = ++g_seat->serial;
                        send_pointer_axis(time_msec, WL_POINTER_AXIS_HORIZONTAL_SCROLL,
                                          wl_fixed_from_int(ev.value * 10));
                    }
                } else if (ev.type == EV_ABS) {
                    if (ev.code == ABS_X) {
                        pointer_x = wl_fixed_from_int(ev.value);
                    } else if (ev.code == ABS_Y) {
                        pointer_y = wl_fixed_from_int(ev.value);
                    }
                } else if (ev.type == EV_KEY) {
                    uint32_t button = 0;
                    switch (ev.code) {
                        case BTN_LEFT:   button = BTN_LEFT;   break;
                        case BTN_RIGHT:  button = BTN_RIGHT;  break;
                        case BTN_MIDDLE: button = BTN_MIDDLE; break;
                        default: continue;
                    }
                    uint32_t button_state = (ev.value == 1) ?
                        WL_POINTER_BUTTON_STATE_PRESSED :
                        WL_POINTER_BUTTON_STATE_RELEASED;

                    if (button_state == WL_POINTER_BUTTON_STATE_PRESSED) {
                        /* Set grab on button press. */
                        g_seat->grab_surface_resource =
                            g_seat->pointer_surface_resource;
                    } else {
                        /* Clear grab on button release. */
                        g_seat->grab_surface_resource = nullptr;
                    }

                    serial = ++g_seat->serial;
                    send_pointer_button(serial, time_msec, button, button_state);
                }
            }
        }
    }

    // Send motion if we processed any events
    if (time_msec > 0) {
        serial = ++g_seat->serial;
        pointer_check_focus(serial);
        send_pointer_motion(time_msec);
        send_pointer_axis_done(time_msec);
    }
}

void input_destroy(void) {
    for (int i = 0; i < input_device_count; i++) {
        if (input_devices[i].fd >= 0) {
            close(input_devices[i].fd);
            input_devices[i].fd = -1;
        }
    }
    input_device_count = 0;
}

/* ---------- Input script execution (P1-T06-E) ---------- */

/* Execute an input script file. Returns 0 on success, -1 on error,
   1 if the script requested quit. */
int input_execute_script(const char* filename,
                          struct MansionSeat* seat,
                          struct MansionCompositor* comp) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Failed to open input script: %s\n", filename);
        return -1;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        /* Trim trailing newline. */
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }
        if (len == 0) continue;

        /* Skip comments. */
        if (line[0] == '#') continue;

        if (strncmp(line, "wait ", 5) == 0) {
            long ms = strtol(line + 5, nullptr, 10);
            if (ms > 0) {
                struct timespec ts = {ms / 1000, (ms % 1000) * 1000000L};
                nanosleep(&ts, nullptr);
            }
        } else if (strncmp(line, "key ", 4) == 0) {
            /* "key CODE press|release" — CODE is a Linux keycode (e.g. 30 = A). */
            char code_str[16] = {};
            char action[16] = {};
            sscanf(line + 4, "%15s %15s", code_str, action);
            int keycode = strtol(code_str, nullptr, 10);
            uint32_t state = (strcmp(action, "press") == 0) ?
                WL_KEYBOARD_KEY_STATE_PRESSED :
                WL_KEYBOARD_KEY_STATE_RELEASED;
            uint32_t serial = ++seat->serial;
            uint32_t time = 0;

            /* Update xkb state. */
            if (seat->xkbstate) {
                xkb_state_update_key(seat->xkbstate,
                                     keycode + 8,
                                     state == WL_KEYBOARD_KEY_STATE_PRESSED ?
                                         XKB_KEY_DOWN : XKB_KEY_UP);
            }

            SeatKeyboardClient *kc, *kc_next;
            wl_list_for_each_safe(kc, kc_next, &seat->keyboard_clients, destroy_listener.link) {
                if (kc->resource) {
                    wl_keyboard_send_key(kc->resource, serial, time, keycode, state);
                }
            }
            send_keymap_modifiers(serial);
        } else if (strncmp(line, "motion ", 7) == 0) {
            int x = 0, y = 0;
            sscanf(line + 7, "%d %d", &x, &y);
            pointer_x = wl_fixed_from_int(x);
            pointer_y = wl_fixed_from_int(y);
            uint32_t serial = ++seat->serial;
            pointer_check_focus(serial);
            send_pointer_motion(0);
            send_pointer_axis_done(0);
        } else if (strncmp(line, "button ", 7) == 0) {
            /* "button CODE press|release" — CODE is a Linux button code. */
            char code_str[16] = {};
            char action[16] = {};
            sscanf(line + 7, "%15s %15s", code_str, action);
            uint32_t button = (uint32_t)strtol(code_str, nullptr, 10);
            uint32_t button_state = (strcmp(action, "press") == 0) ?
                WL_POINTER_BUTTON_STATE_PRESSED :
                WL_POINTER_BUTTON_STATE_RELEASED;

            if (button_state == WL_POINTER_BUTTON_STATE_PRESSED) {
                seat->grab_surface_resource = seat->pointer_surface_resource;
            } else {
                seat->grab_surface_resource = nullptr;
            }

            uint32_t serial = ++seat->serial;
            send_pointer_button(serial, 0, button, button_state);
        } else if (strcmp(line, "focus gained") == 0) {
            /* Focus gained: re-send enter to the currently focused surface. */
            if (comp && comp->focused_surface_resource) {
                seat_set_keyboard_focus(seat, comp->focused_surface_resource, comp);
            }
        } else if (strcmp(line, "focus lost") == 0) {
            seat_set_keyboard_focus(seat, nullptr, comp);
        } else if (strcmp(line, "quit") == 0) {
            fclose(fp);
            return 1;
        }
    }

    fclose(fp);
    return 0;
}

/* ---------- SeatKeyboardClient destroy listener ---------- */

static void keyboard_client_destroy(struct wl_listener* listener, void* data) {
    (void)data;
    SeatKeyboardClient* kbc = wl_container_of(listener, kbc, destroy_listener);
    wl_list_remove(&kbc->destroy_listener.link);
    kbc->resource = nullptr;
    delete kbc;
}

/* ---------- SeatPointerClient destroy listener ---------- */

static void pointer_client_destroy(struct wl_listener* listener, void* data) {
    (void)data;
    SeatPointerClient* pkc = wl_container_of(listener, pkc, destroy_listener);
    wl_list_remove(&pkc->destroy_listener.link);
    pkc->resource = nullptr;
    delete pkc;
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
    wl_signal_add(&resource->destroy_signal, destroy_listener);
    destroy_listener->notify = pointer_client_destroy;

    wl_list_insert(&seat->pointer_clients, &kpc->destroy_listener.link);

    // Send enter event (pointer enters the first surface)
    // In a real compositor, this would do a hit-test. For now, send a synthetic enter.
    wl_pointer_send_enter(resource, 0, nullptr,
                          pointer_x, pointer_y);

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
    wl_signal_add(&resource->destroy_signal, destroy_listener);
    destroy_listener->notify = keyboard_client_destroy;

    wl_list_insert(&seat->keyboard_clients, &kbc->destroy_listener.link);

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
    auto* seat = new MansionSeat;
    memset(seat, 0, sizeof(*seat));

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

    // Destroy all keyboard clients
    SeatKeyboardClient *kc, *kc_next;
    wl_list_for_each_safe(kc, kc_next, &seat->keyboard_clients, destroy_listener.link) {
        wl_list_remove(&kc->destroy_listener.link);
        if (kc->resource) {
            wl_resource_destroy(kc->resource);
        }
        delete kc;
    }
    wl_list_init(&seat->keyboard_clients);

    // Destroy all pointer clients
    SeatPointerClient *pk, *pk_next;
    wl_list_for_each_safe(pk, pk_next, &seat->pointer_clients, destroy_listener.link) {
        wl_list_remove(&pk->destroy_listener.link);
        if (pk->resource) {
            wl_resource_destroy(pk->resource);
        }
        delete pk;
    }
    wl_list_init(&seat->pointer_clients);

    /* Clear pointer and keyboard focus. */
    seat->grab_surface_resource = nullptr;
    seat->pointer_surface_resource = nullptr;
    seat->focused_surface_resource = nullptr;

    wl_global_destroy(seat->global);

    // Clean up xkb state and keymap
    if (seat->xkbstate) {
        xkb_state_unref(seat->xkbstate);
    }
    if (seat->keymap) {
        xkb_keymap_unref(seat->keymap);
    }

    delete seat;
}

/* ---------- Keyboard focus (P1-T06-C) ---------- */

void seat_set_keyboard_focus(struct MansionSeat* seat,
                              struct wl_resource* surface,
                              MansionCompositor* comp) {
    if (!seat || !comp) return;

    comp->keyboard_focus_serial++;

    /* Send leave to the old focused surface. */
    if (comp->focused_surface_resource) {
        SeatKeyboardClient *kc, *kc_next;
        wl_list_for_each_safe(kc, kc_next, &seat->keyboard_clients, destroy_listener.link) {
            if (kc->resource) {
                wl_keyboard_send_leave(kc->resource,
                                       comp->keyboard_focus_serial,
                                       comp->focused_surface_resource);
            }
        }
    }

    comp->focused_surface_resource = surface;
    seat->focused_surface_resource = surface;

    /* Send enter to the new surface. */
    if (surface) {
        struct wl_array keys;
        wl_array_init(&keys);
        SeatKeyboardClient *kc, *kc_next;
        wl_list_for_each_safe(kc, kc_next, &seat->keyboard_clients, destroy_listener.link) {
            if (kc->resource) {
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
