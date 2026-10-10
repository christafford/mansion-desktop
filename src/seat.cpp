#include "seat.h"
#include "seat-private.h"
#include "compositor-private.h"
#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <new>
#include <chrono>
#include <cmath>
#include <linux/input-event-codes.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-server-protocol.h>

static void pointer_frame(wl_resource* resource) {
    if (wl_resource_get_version(resource) >= WL_POINTER_FRAME_SINCE_VERSION) wl_pointer_send_frame(resource);
}
static void keyboard_enter(ElsewhereSeat* seat, wl_resource* keyboard, uint32_t serial);
static void keyboard_focus_destroyed(wl_listener* listener, void* data);
static void pointer_focus_destroyed(wl_listener* listener, void* data);

/* ---------- SeatKeyboardClient destroy listener ---------- */

static void keyboard_client_destroy(struct wl_listener* listener, void* data) {
    (void)data;
    SeatKeyboardClient* kbc = wl_container_of(listener, kbc, destroy_listener);
    wl_list_remove(&kbc->destroy_listener.link);
    wl_list_remove(&kbc->seat_link);
    delete kbc;
}

/* ---------- SeatPointerClient destroy listener ---------- */

static void pointer_client_destroy(struct wl_listener* listener, void* data) {
    (void)data;
    SeatPointerClient* pkc = wl_container_of(listener, pkc, destroy_listener);
    wl_list_remove(&pkc->destroy_listener.link);
    wl_list_remove(&pkc->seat_link);
    delete pkc;
}

/* ---------- Pointer implementation ---------- */

static void pointer_set_cursor(struct wl_client* client, struct wl_resource* resource,
                                uint32_t serial, struct wl_resource* surface,
                                int32_t x, int32_t y) {
    (void)client; (void)resource; (void)serial; (void)surface; (void)x; (void)y;
}

static void resource_release(wl_client*, wl_resource* resource) {
    wl_resource_destroy(resource);
}
static const struct wl_pointer_interface pointer_impl = {
    pointer_set_cursor,
    resource_release,
};

/* ---------- Keyboard implementation ---------- */

static const struct wl_keyboard_interface keyboard_impl = {
    resource_release,
};

/* ---------- Seat implementations ---------- */

static void seat_get_pointer(struct wl_client* client, struct wl_resource* seat_resource,
                               uint32_t id) {
    auto* seat = static_cast<ElsewhereSeat*>(wl_resource_get_user_data(seat_resource));

    auto* kpc = new (std::nothrow) SeatPointerClient{};
    if (!kpc) { wl_client_post_no_memory(client); return; }

    auto* resource = wl_resource_create(client, &wl_pointer_interface, wl_resource_get_version(seat_resource), id);
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
    wl_resource_add_destroy_listener(resource, destroy_listener);

    // Track in seat's client list using a separate link
    wl_list_insert(&seat->pointer_clients, &kpc->seat_link);

    // Only the owned runtime installs this lifetime-tracked focus listener.
    if (!wl_list_empty(&seat->pointer_focus_destroy.link) && seat->pointer_surface_resource &&
        wl_resource_get_client(seat->pointer_surface_resource) == client) {
        wl_pointer_send_enter(resource, wl_display_next_serial(seat->display),
            seat->pointer_surface_resource, wl_fixed_from_double(seat->pointer_x),
            wl_fixed_from_double(seat->pointer_y));
        pointer_frame(resource);
    }
}

static void seat_get_keyboard(struct wl_client* client, struct wl_resource* seat_resource,
                               uint32_t id) {
    auto* seat = static_cast<ElsewhereSeat*>(wl_resource_get_user_data(seat_resource));

    auto* kbc = new (std::nothrow) SeatKeyboardClient{};
    if (!kbc) { wl_client_post_no_memory(client); return; }

    auto* resource = wl_resource_create(client, &wl_keyboard_interface, wl_resource_get_version(seat_resource), id);
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
    wl_resource_add_destroy_listener(resource, destroy_listener);

    // Track in seat's client list using a separate link
    wl_list_insert(&seat->keyboard_clients, &kbc->seat_link);

    // The fd includes the terminating NUL required by wl_keyboard.keymap.
    char* text = xkb_keymap_get_as_string(seat->keymap, XKB_KEYMAP_FORMAT_TEXT_V1);
    if (!text) { wl_client_post_no_memory(client); return; }
    const size_t size = strlen(text) + 1;
    int fd = memfd_create("elsewhere-keymap", MFD_CLOEXEC);
    if (fd < 0) {
        free(text);
        wl_client_post_implementation_error(client, "could not create keymap fd: %s", strerror(errno));
        return;
    }
    size_t offset = 0;
    while (offset < size) {
        ssize_t written = write(fd, text + offset, size - offset);
        if (written < 0 && errno == EINTR) continue;
        if (written <= 0) break;
        offset += static_cast<size_t>(written);
    }
    free(text);
    if (offset != size) {
        close(fd);
        wl_client_post_implementation_error(client, "could not write keymap fd");
        return;
    }
    wl_keyboard_send_keymap(resource, WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1, fd, size);
    close(fd);
    if (wl_resource_get_version(resource) >= WL_KEYBOARD_REPEAT_INFO_SINCE_VERSION)
        wl_keyboard_send_repeat_info(resource, 25, 500);
    // A keyboard bound after activation must receive the current focus too.
    if (seat->focused_surface_resource &&
        wl_resource_get_client(seat->focused_surface_resource) == client)
        keyboard_enter(seat, resource, wl_display_next_serial(seat->display));
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

static void seat_resource_destroyed(wl_resource* resource) {
    wl_list_remove(wl_resource_get_link(resource));
}
static void seat_bind(struct wl_client* client, void* data, uint32_t version, uint32_t id) {
    auto* seat = static_cast<ElsewhereSeat*>(data);

    auto* resource = wl_resource_create(client, &wl_seat_interface, std::min(version, 5u), id);
    if (!resource) {
        wl_client_post_no_memory(client);
        return;
    }

    wl_resource_set_implementation(resource, &seat_impl, seat, seat_resource_destroyed);
    wl_list_insert(&seat->resources, wl_resource_get_link(resource));

    // Send seat name
    if (version >= WL_SEAT_NAME_SINCE_VERSION)
        wl_seat_send_name(resource, "default-seat");

    // Send capabilities (pointer + keyboard)
    wl_seat_send_capabilities(resource, WL_SEAT_CAPABILITY_POINTER | WL_SEAT_CAPABILITY_KEYBOARD);
}

struct ElsewhereSeat* seat_create(struct wl_display* display, uint32_t version) {
    auto* seat = new (std::nothrow) ElsewhereSeat();
    if (!seat) return nullptr;
    seat->display = display;
    wl_list_init(&seat->keyboard_focus_destroy.link);
    seat->keyboard_focus_destroy.notify = keyboard_focus_destroyed;
    wl_list_init(&seat->pointer_focus_destroy.link);
    seat->pointer_focus_destroy.notify = pointer_focus_destroyed;
    wl_list_init(&seat->resources);
    wl_list_init(&seat->keyboard_clients);
    wl_list_init(&seat->pointer_clients);
    seat->serial = 1;

    seat->global = wl_global_create(display, &wl_seat_interface, std::min(version, 5u), seat, seat_bind);
    if (!seat->global) {
        delete seat;
        return nullptr;
    }

    xkb_context* context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (context) {
        xkb_rule_names names{};
        names.layout = "us";
        seat->keymap = xkb_keymap_new_from_names(context, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
        xkb_context_unref(context);
        if (seat->keymap) seat->xkbstate = xkb_state_new(seat->keymap);
    }
    if (!seat->keymap || !seat->xkbstate) {
        seat_destroy(seat);
        errno = EINVAL;
        return nullptr;
    }

    return seat;
}

void seat_destroy(struct ElsewhereSeat* seat) {
    if (!seat) return;
    wl_list_remove(&seat->keyboard_focus_destroy.link);
    wl_list_remove(&seat->pointer_focus_destroy.link);

    // Clear focus before destroying resources to avoid stale references.
    seat->grab_surface_resource = nullptr;
    seat->pointer_surface_resource = nullptr;
    seat->focused_surface_resource = nullptr;

    while (!wl_list_empty(&seat->keyboard_clients)) {
        SeatKeyboardClient* client = wl_container_of(seat->keyboard_clients.next, client, seat_link);
        wl_resource_destroy(client->resource);
    }
    while (!wl_list_empty(&seat->pointer_clients)) {
        SeatPointerClient* client = wl_container_of(seat->pointer_clients.next, client, seat_link);
        wl_resource_destroy(client->resource);
    }
    while (!wl_list_empty(&seat->resources))
        wl_resource_destroy(wl_resource_from_link(seat->resources.next));

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

/* ---------- Focused keyboard delivery ---------- */

static uint32_t keyboard_time() {
    return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

static void keyboard_modifiers(ElsewhereSeat* seat, wl_resource* resource, uint32_t serial) {
    wl_keyboard_send_modifiers(resource, serial,
        xkb_state_serialize_mods(seat->xkbstate, XKB_STATE_MODS_DEPRESSED),
        xkb_state_serialize_mods(seat->xkbstate, XKB_STATE_MODS_LATCHED),
        xkb_state_serialize_mods(seat->xkbstate, XKB_STATE_MODS_LOCKED),
        xkb_state_serialize_layout(seat->xkbstate, XKB_STATE_LAYOUT_EFFECTIVE));
}

static void keyboard_enter(ElsewhereSeat* seat, wl_resource* keyboard, uint32_t serial) {
    wl_array keys;
    wl_array_init(&keys);
    if (!seat->pressed_keys.empty()) {
        auto* data = wl_array_add(&keys, seat->pressed_keys.size() * sizeof(uint32_t));
        if (!data) { wl_resource_post_no_memory(keyboard); wl_array_release(&keys); return; }
        std::memcpy(data, seat->pressed_keys.data(), keys.size);
    }
    wl_keyboard_send_enter(keyboard, serial, seat->focused_surface_resource, &keys);
    wl_array_release(&keys);
    keyboard_modifiers(seat, keyboard, serial);
}

bool seat_keyboard_key(ElsewhereSeat* seat, uint32_t keycode, bool pressed) {
    if (!seat || !seat->focused_surface_resource || keycode == 0 || keycode > KEY_MAX) return false;
    auto found = std::find(seat->pressed_keys.begin(), seat->pressed_keys.end(), keycode);
    if (pressed == (found != seat->pressed_keys.end())) return true;
    if (pressed) seat->pressed_keys.push_back(keycode);
    else seat->pressed_keys.erase(found);
    const auto changed = xkb_state_update_key(seat->xkbstate, keycode + 8,
        pressed ? XKB_KEY_DOWN : XKB_KEY_UP);
    const uint32_t serial = wl_display_next_serial(seat->display);
    const uint32_t time = keyboard_time();
    auto* client = wl_resource_get_client(seat->focused_surface_resource);
    SeatKeyboardClient* kc;
    wl_list_for_each(kc, &seat->keyboard_clients, seat_link) {
        if (wl_resource_get_client(kc->resource) != client) continue;
        wl_keyboard_send_key(kc->resource, serial, time, keycode,
            pressed ? WL_KEYBOARD_KEY_STATE_PRESSED : WL_KEYBOARD_KEY_STATE_RELEASED);
        if (changed) keyboard_modifiers(seat, kc->resource, serial);
    }
    return true;
}

static void keyboard_release_all(ElsewhereSeat* seat) {
    while (!seat->pressed_keys.empty()) {
        const uint32_t key = seat->pressed_keys.back();
        if (seat->focused_surface_resource) seat_keyboard_key(seat, key, false);
        else {
            xkb_state_update_key(seat->xkbstate, key + 8, XKB_KEY_UP);
            seat->pressed_keys.pop_back();
        }
    }
    // Focus boundaries reset locks/latched modifiers as well as held keys.
    xkb_state_update_mask(seat->xkbstate, 0, 0, 0, 0, 0, 0);
    if (seat->focused_surface_resource) {
        auto* client = wl_resource_get_client(seat->focused_surface_resource);
        const uint32_t serial = wl_display_next_serial(seat->display);
        SeatKeyboardClient* kc;
        wl_list_for_each(kc, &seat->keyboard_clients, seat_link)
            if (wl_resource_get_client(kc->resource) == client)
                keyboard_modifiers(seat, kc->resource, serial);
    }
}

static void keyboard_focus_destroyed(wl_listener* listener, void* data) {
    ElsewhereSeat* seat = wl_container_of(listener, seat, keyboard_focus_destroy);
    // The surface proxy is already gone. Never send a leave referencing it.
    keyboard_release_all(seat);
    seat->focused_surface_resource = nullptr;
    if (seat->keyboard_compositor && seat->keyboard_compositor->focused_surface_resource == data)
        seat->keyboard_compositor->focused_surface_resource = nullptr;
    wl_list_remove(&seat->keyboard_focus_destroy.link);
    wl_list_init(&seat->keyboard_focus_destroy.link);
}

void seat_set_keyboard_focus(ElsewhereSeat* seat, wl_resource* surface, ElsewhereCompositor* comp) {
    if (!seat || !comp) return;
    if (seat->focused_surface_resource == surface && comp->focused_surface_resource == surface) return;
    keyboard_release_all(seat);
    const uint32_t serial = wl_display_next_serial(seat->display);
    if (seat->focused_surface_resource) {
        auto* client = wl_resource_get_client(seat->focused_surface_resource);
        SeatKeyboardClient* kc;
        wl_list_for_each(kc, &seat->keyboard_clients, seat_link)
            if (wl_resource_get_client(kc->resource) == client)
                wl_keyboard_send_leave(kc->resource, serial, seat->focused_surface_resource);
    }
    wl_list_remove(&seat->keyboard_focus_destroy.link);
    wl_list_init(&seat->keyboard_focus_destroy.link);
    comp->keyboard_focus_serial = serial;
    comp->focused_surface_resource = surface;
    seat->focused_surface_resource = surface;
    seat->keyboard_compositor = comp;
    if (surface) {
        wl_resource_add_destroy_listener(surface, &seat->keyboard_focus_destroy);
        auto* client = wl_resource_get_client(surface);
        SeatKeyboardClient* kc;
        wl_list_for_each(kc, &seat->keyboard_clients, seat_link)
            if (wl_resource_get_client(kc->resource) == client) keyboard_enter(seat, kc->resource, serial);
    }
}

void compositor_set_seat(struct ElsewhereCompositor* compositor,
                          struct ElsewhereSeat* seat) {
    if (!compositor) return;
    compositor->seat = seat;
}

/* ---------- Owned runtime pointer delivery (seat v1-v5) ---------- */

static bool pointer_value(double value) {
    // Leave ample room inside wl_fixed's signed 24.8 range before conversion.
    return std::isfinite(value) && std::abs(value) <= 1000000.0;
}

wl_resource* seat_pointer_surface(ElsewhereSeat* seat) {
    return seat ? seat->pointer_surface_resource : nullptr;
}
bool seat_pointer_grabbed(ElsewhereSeat* seat) {
    return seat && !seat->pressed_buttons.empty();
}

bool seat_pointer_button(ElsewhereSeat* seat, uint32_t button, bool pressed) {
    if (!seat || !seat->pointer_surface_resource || button < BTN_LEFT || button > BTN_TASK) return false;
    auto found = std::find(seat->pressed_buttons.begin(), seat->pressed_buttons.end(), button);
    if (pressed == (found != seat->pressed_buttons.end())) return true;
    if (pressed) seat->pressed_buttons.push_back(button);
    else seat->pressed_buttons.erase(found);
    seat->grab_surface_resource = seat->pressed_buttons.empty() ? nullptr : seat->pointer_surface_resource;
    const auto serial = wl_display_next_serial(seat->display);
    const auto time = keyboard_time();
    auto* client = wl_resource_get_client(seat->pointer_surface_resource);
    SeatPointerClient* pc;
    wl_list_for_each(pc, &seat->pointer_clients, seat_link)
        if (wl_resource_get_client(pc->resource) == client) {
            wl_pointer_send_button(pc->resource, serial, time, button,
                pressed ? WL_POINTER_BUTTON_STATE_PRESSED : WL_POINTER_BUTTON_STATE_RELEASED);
            pointer_frame(pc->resource);
        }
    return true;
}

static void pointer_release_all(ElsewhereSeat* seat) {
    while (!seat->pressed_buttons.empty()) {
        if (seat->pointer_surface_resource) seat_pointer_button(seat, seat->pressed_buttons.back(), false);
        else seat->pressed_buttons.pop_back();
    }
    seat->grab_surface_resource = nullptr;
}

static void pointer_focus_destroyed(wl_listener* listener, void*) {
    ElsewhereSeat* seat = wl_container_of(listener, seat, pointer_focus_destroy);
    // Releases contain no surface reference; leave would refer to a dead proxy.
    pointer_release_all(seat);
    seat->pointer_surface_resource = nullptr;
    wl_list_remove(&seat->pointer_focus_destroy.link);
    wl_list_init(&seat->pointer_focus_destroy.link);
}

void seat_pointer_reset(ElsewhereSeat* seat) {
    if (!seat) return;
    pointer_release_all(seat);
    if (seat->pointer_surface_resource) {
        auto* client = wl_resource_get_client(seat->pointer_surface_resource);
        const auto serial = wl_display_next_serial(seat->display);
        SeatPointerClient* pc;
        wl_list_for_each(pc, &seat->pointer_clients, seat_link)
            if (wl_resource_get_client(pc->resource) == client) {
                wl_pointer_send_leave(pc->resource, serial, seat->pointer_surface_resource);
                pointer_frame(pc->resource);
            }
    }
    seat->pointer_surface_resource = nullptr;
    wl_list_remove(&seat->pointer_focus_destroy.link);
    wl_list_init(&seat->pointer_focus_destroy.link);
}

bool seat_pointer_motion(ElsewhereSeat* seat, wl_resource* surface, double x, double y) {
    if (!seat || !pointer_value(x) || !pointer_value(y)) return false;
    // The caller must keep using the grabbed surface's coordinate system.
    if (seat_pointer_grabbed(seat) && surface != seat->pointer_surface_resource) return false;
    const bool changed = seat->pointer_surface_resource != surface;
    if (changed) seat_pointer_reset(seat);
    seat->pointer_x = x; seat->pointer_y = y;
    if (!surface) return true;
    if (changed) {
        seat->pointer_surface_resource = surface;
        wl_resource_add_destroy_listener(surface, &seat->pointer_focus_destroy);
    }
    const auto serial = changed ? wl_display_next_serial(seat->display) : 0;
    const auto time = keyboard_time();
    auto* client = wl_resource_get_client(surface);
    SeatPointerClient* pc;
    wl_list_for_each(pc, &seat->pointer_clients, seat_link) {
        if (wl_resource_get_client(pc->resource) != client) continue;
        if (changed) wl_pointer_send_enter(pc->resource, serial, surface, wl_fixed_from_double(x), wl_fixed_from_double(y));
        else wl_pointer_send_motion(pc->resource, time, wl_fixed_from_double(x), wl_fixed_from_double(y));
        pointer_frame(pc->resource);
    }
    return true;
}

bool seat_pointer_axis(ElsewhereSeat* seat, double horizontal, double vertical) {
    if (!seat || !seat->pointer_surface_resource || !pointer_value(horizontal) || !pointer_value(vertical)) return false;
    const auto time = keyboard_time();
    auto* client = wl_resource_get_client(seat->pointer_surface_resource);
    SeatPointerClient* pc;
    wl_list_for_each(pc, &seat->pointer_clients, seat_link) {
        if (wl_resource_get_client(pc->resource) != client) continue;
        if (wl_resource_get_version(pc->resource) >= WL_POINTER_AXIS_SOURCE_SINCE_VERSION)
            wl_pointer_send_axis_source(pc->resource, WL_POINTER_AXIS_SOURCE_WHEEL);
        if (horizontal) wl_pointer_send_axis(pc->resource, time, WL_POINTER_AXIS_HORIZONTAL_SCROLL, wl_fixed_from_double(horizontal));
        if (vertical) wl_pointer_send_axis(pc->resource, time, WL_POINTER_AXIS_VERTICAL_SCROLL, wl_fixed_from_double(vertical));
        pointer_frame(pc->resource);
    }
    return true;
}
