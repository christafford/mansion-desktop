#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <wayland-server-protocol.h>
#include <xkbcommon/xkbcommon.h>

#include "compositor.h"
#include "display.h"
#include "input.h"

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

static void keyboard_handle_key(void* data, struct wl_keyboard* wl_keyboard,
                                 uint32_t serial, uint32_t time, uint32_t key,
                                 uint32_t state) {
    (void)wl_keyboard; (void)time; (void)serial;
    auto* seat = static_cast<MansionSeat*>(data);
    (void)key; (void)state;
    (void)seat;
}

static void keyboard_handle_modifiers(void* data, struct wl_keyboard* wl_keyboard,
                                       uint32_t serial, uint32_t mod_depressed,
                                       uint32_t mod_latched, uint32_t mod_locked,
                                       uint32_t group) {
    (void)wl_keyboard; (void)serial; (void)mod_depressed;
    (void)mod_latched; (void)mod_locked; (void)group;
    auto* seat = static_cast<MansionSeat*>(data);
    (void)seat;
}

static void keyboard_handle_enter(void* data, struct wl_keyboard* wl_keyboard,
                                   uint32_t serial, struct wl_resource* surface,
                                   struct wl_array* keys) {
    (void)data; (void)wl_keyboard; (void)serial; (void)surface; (void)keys;
}

static void keyboard_handle_leave(void* data, struct wl_keyboard* wl_keyboard,
                                   uint32_t serial, struct wl_resource* surface) {
    (void)data; (void)wl_keyboard; (void)serial; (void)surface;
}

static const struct wl_keyboard_interface keyboard_impl = {
    nullptr, /* release (v3, optional) */
};

static void pointer_handle_motion(void* data, struct wl_pointer* wl_pointer,
                                   uint32_t time, wl_fixed_t x, wl_fixed_t y) {
    (void)wl_pointer; (void)time;
    auto* seat = static_cast<MansionSeat*>(data);
    seat->pointer.x = x;
    seat->pointer.y = y;
}

static void pointer_handle_button(void* data, struct wl_pointer* wl_pointer,
                                   uint32_t time, uint32_t button, uint32_t state) {
    (void)data; (void)wl_pointer; (void)time; (void)button; (void)state;
}

static void pointer_handle_axis(void* data, struct wl_pointer* wl_pointer,
                                 uint32_t time, uint32_t axis, wl_fixed_t value) {
    (void)data; (void)wl_pointer; (void)time; (void)axis; (void)value;
}

static void pointer_handle_enter(void* data, struct wl_pointer* wl_pointer,
                                  uint32_t serial, struct wl_resource* surface,
                                  wl_fixed_t x, wl_fixed_t y) {
    (void)data; (void)wl_pointer; (void)serial; (void)surface; (void)x; (void)y;
}

static void pointer_handle_leave(void* data, struct wl_pointer* wl_pointer,
                                  uint32_t serial, struct wl_resource* surface) {
    (void)data; (void)wl_pointer; (void)serial; (void)surface;
}

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

    return seat;
}

void destroy_seat(struct MansionSeat* seat) {
    if (!seat) return;

    wl_global_destroy(seat->global);

    if (seat->keyboard.xkbstate) {
        xkb_state_unref(seat->keyboard.xkbstate);
    }
    if (seat->keyboard.keymap) {
        xkb_keymap_unref(seat->keyboard.keymap);
    }

    delete seat;
}
