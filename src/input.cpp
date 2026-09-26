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

#include <wayland-server-protocol.h>
#include <xkbcommon/xkbcommon.h>

#include "compositor.h"
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
static struct MansionSeat* g_seat = nullptr;

struct MansionSeat {
    struct wl_global* global;
    struct wl_resource* seat_resource;

    struct {
        struct wl_resource* resource;
        xkb_keymap* keymap;
        xkb_state* xkbstate;
    } keyboard;

    struct {
        struct wl_resource* resource;
        wl_fixed_t x;
        wl_fixed_t y;
    } pointer;
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

void input_process(void) {
    if (!g_seat) return;

    uint32_t time_msec = 0;
    uint32_t serial = 1;  // Simple serial counter
    auto* pointer_resource = g_seat->pointer.resource;

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
                if (g_seat->keyboard.resource) {
                    wl_keyboard_send_key(g_seat->keyboard.resource,
                                         serial, time_msec, ev.code, state);
                }
            }

            // Handle mouse events
            if (input_devices[i].is_mouse) {
                if (ev.type == EV_REL) {
                    if (ev.code == REL_X) {
                        pointer_x += wl_fixed_from_int(ev.value);
                    } else if (ev.code == REL_Y) {
                        pointer_y += wl_fixed_from_int(ev.value);
                    } else if (ev.code == REL_WHEEL) {
                        wl_pointer_send_axis(pointer_resource, time_msec,
                                             WL_POINTER_AXIS_VERTICAL_SCROLL,
                                             wl_fixed_from_int(ev.value * 10));
                    } else if (ev.code == REL_HWHEEL) {
                        wl_pointer_send_axis(pointer_resource, time_msec,
                                             WL_POINTER_AXIS_HORIZONTAL_SCROLL,
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
                    wl_pointer_send_button(pointer_resource, serial, time_msec,
                                            button, button_state);
                }
            }
        }
    }

    // Send motion if we processed any events
    if (time_msec > 0 && pointer_resource) {
        wl_pointer_send_motion(pointer_resource, time_msec,
                               pointer_x, pointer_y);
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

static const struct wl_keyboard_interface keyboard_impl = {
    nullptr, /* release (v3, optional) */
};

static void pointer_set_cursor(struct wl_client* client, struct wl_resource* resource,
                                uint32_t serial, struct wl_resource* surface,
                                int32_t x, int32_t y) {
    (void)client; (void)resource; (void)serial; (void)surface; (void)x; (void)y;
}

static const struct wl_pointer_interface pointer_impl = {
    pointer_set_cursor,
    nullptr, /* release (v4, optional) */
};

static void seat_get_pointer(struct wl_client* client, struct wl_resource* seat_resource,
                              uint32_t id) {
    auto* seat = static_cast<MansionSeat*>(wl_resource_get_user_data(seat_resource));

    if (seat->pointer.resource) {
        wl_resource_post_error(seat_resource, WL_SEAT_ERROR_MISSING_CAPABILITY,
                               "pointer already created");
        return;
    }

    auto* resource = wl_resource_create(client, &wl_pointer_interface, 4, id);
    if (!resource) {
        wl_client_post_no_memory(client);
        return;
    }

    wl_resource_set_implementation(resource, &pointer_impl, seat, nullptr);
    seat->pointer.resource = resource;
}

static void seat_get_keyboard(struct wl_client* client, struct wl_resource* seat_resource,
                               uint32_t id) {
    auto* seat = static_cast<MansionSeat*>(wl_resource_get_user_data(seat_resource));

    if (seat->keyboard.resource) {
        wl_resource_post_error(seat_resource, WL_SEAT_ERROR_MISSING_CAPABILITY,
                               "keyboard already created");
        return;
    }

    auto* resource = wl_resource_create(client, &wl_keyboard_interface, 7, id);
    if (!resource) {
        wl_client_post_no_memory(client);
        return;
    }

    wl_resource_set_implementation(resource, &keyboard_impl, seat, nullptr);
    seat->keyboard.resource = resource;

    // Create xkb keymap and send it via fd
    seat->keyboard.keymap = xkb_keymap_new_from_string(
        nullptr,
        "xkb_keymap {\n"
        "    xkb_keycodes  { include \"evdev+aliases(qwerty)\" };\n"
        "    xkb_types     { include \"complete\" };\n"
        "    xkb_compat    { include \"complete\" };\n"
        "    xkb_symbols   { include \"pc+us+inet(evdev)\" };\n"
        "    xkb_geometry  { include \"pc(pc105)\" };\n"
        "};\n",
        XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);

    if (seat->keyboard.keymap) {
        seat->keyboard.xkbstate = xkb_state_new(seat->keyboard.keymap);

        // Write keymap to memfd and send fd to client
        const char* keymap_string = xkb_keymap_get_as_string(
            seat->keyboard.keymap, XKB_KEYMAP_FORMAT_TEXT_V1);

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
    seat->seat_resource = resource;

    // Send seat name
    wl_seat_send_name(resource, "default-seat");

    // Send capabilities (pointer + keyboard)
    wl_seat_send_capabilities(resource, WL_SEAT_CAPABILITY_POINTER | WL_SEAT_CAPABILITY_KEYBOARD);
}

struct MansionSeat* create_seat(struct wl_display* display) {
    auto* seat = new MansionSeat;
    memset(seat, 0, sizeof(*seat));

    seat->global = wl_global_create(display, &wl_seat_interface, 4, seat, seat_bind);
    if (!seat->global) {
        delete seat;
        return nullptr;
    }

    // Initialize keyboard state
    seat->keyboard.keymap = nullptr;
    seat->keyboard.xkbstate = nullptr;

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

    wl_global_destroy(seat->global);

    // Clean up xkb state and keymap
    if (seat->keyboard.xkbstate) {
        xkb_state_unref(seat->keyboard.xkbstate);
    }
    if (seat->keyboard.keymap) {
        xkb_keymap_unref(seat->keyboard.keymap);
    }

    delete seat;
}
