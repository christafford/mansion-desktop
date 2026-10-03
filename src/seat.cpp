#include "seat.h"
#include "seat-private.h"
#include "compositor-private.h"
#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <new>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-server-protocol.h>

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
    auto* seat = static_cast<MansionSeat*>(wl_resource_get_user_data(seat_resource));

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

    /* Don't send a synthetic enter here; proper enter/leave is handled
       by pointer_check_focus() when the pointer position changes. */

    // Send capabilities (done at seat level, not per-client)
}

static void seat_get_keyboard(struct wl_client* client, struct wl_resource* seat_resource,
                               uint32_t id) {
    auto* seat = static_cast<MansionSeat*>(wl_resource_get_user_data(seat_resource));

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
    int fd = memfd_create("mansion-keymap", MFD_CLOEXEC);
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
    auto* seat = static_cast<MansionSeat*>(data);

    auto* resource = wl_resource_create(client, &wl_seat_interface, std::min(version, 4u), id);
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

struct MansionSeat* seat_create(struct wl_display* display) {
    auto* seat = new (std::nothrow) MansionSeat();
    if (!seat) return nullptr;
    wl_list_init(&seat->resources);
    wl_list_init(&seat->keyboard_clients);
    wl_list_init(&seat->pointer_clients);
    seat->serial = 1;

    seat->global = wl_global_create(display, &wl_seat_interface, 4, seat, seat_bind);
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

void seat_destroy(struct MansionSeat* seat) {
    if (!seat) return;

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
