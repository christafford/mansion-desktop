#include <cstring>
#include <algorithm>
#include <new>
#include <atomic>
#include <limits>
#include <chrono>
#include "core-frame-state.h"
#include "surface-tree.h"

#include <wayland-server-protocol.h>
#include <wayland-util.h>

#include "compositor-core.h"
#include "compositor-private.h"
#include "xdg-shell.h"

/* ---------- surface ---------- */

static void frame_callback_destroyed(struct wl_resource* resource) {
    wl_list_remove(wl_resource_get_link(resource));
}

static void surface_destroy_callback(struct wl_resource* resource) {
    auto* surface = static_cast<MansionSurface*>(wl_resource_get_user_data(resource));
    if (!surface) return;
    surface_tree_destroyed(surface);
    wl_list_remove(&surface->link);
    wl_list_remove(&surface->buffer_destroy_listener.link);
    wl_list_init(&surface->buffer_destroy_listener.link);
    while (!wl_list_empty(&surface->frame_callback_list)) {
        wl_resource_destroy(wl_resource_from_link(surface->frame_callback_list.next));
    }
    if (surface->compositor->focused_surface_resource == resource) {
        compositor_core_clear_focus(surface->compositor);
    }
    while (!wl_list_empty(&surface->core_frame->committed_callbacks))
        wl_resource_destroy(wl_resource_from_link(surface->core_frame->committed_callbacks.next));
    delete surface->core_frame;
    // A retained OwnedFrame has its own lifetime after the surface disappears.
    wl_resource_set_user_data(resource, nullptr);
    delete surface;
}

static void surface_destroy(struct wl_client* client, struct wl_resource* resource) {
    (void)client;
    wl_resource_destroy(resource);
}

/* Buffer destroy listener — fires when the client destroys the wl_buffer proxy. */
static void buffer_destroy_notify(struct wl_listener* listener, void* data) {
    MansionSurface* surface = wl_container_of(listener, surface, buffer_destroy_listener);
    wl_list_remove(&listener->link);
    wl_list_init(&listener->link);
    if (surface->pending_buffer_resource == data) {
        surface->pending_buffer_resource = nullptr;
        // A destroyed pending buffer is unspecified by Wayland. Preserve the
        // last owned content; only an explicit null attach detaches it.
        surface->core_frame->pending_attach = false;
    }
}

static void surface_attach(struct wl_client* client, struct wl_resource* resource,
                           struct wl_resource* buffer_resource, int32_t x, int32_t y) {
    (void)client; (void)x; (void)y;
    auto* surface = static_cast<MansionSurface*>(wl_resource_get_user_data(resource));

    wl_list_remove(&surface->buffer_destroy_listener.link);
    wl_list_init(&surface->buffer_destroy_listener.link);
    surface->pending_buffer_resource = buffer_resource;
    surface->core_frame->pending_attach = true;
    surface->has_pending_position = true;
    surface->pending_x = x;
    surface->pending_y = y;
    if (buffer_resource) {
        surface->buffer_destroy_listener.notify = buffer_destroy_notify;
        wl_resource_add_destroy_listener(buffer_resource, &surface->buffer_destroy_listener);
    }
}

static void surface_damage(struct wl_client*, struct wl_resource* resource,
                           int32_t, int32_t, int32_t width, int32_t height) {
    // Full-frame copies deliberately subsume damage, without rectangle overflow.
    auto* surface = static_cast<MansionSurface*>(wl_resource_get_user_data(resource));
    if (width > 0 && height > 0) surface->damage_count = 1;
}

static bool copy_frame(MansionSurface* surface, mansion::FrameSnapshot& frame) {
    auto* buffer = wl_shm_buffer_get(surface->pending_buffer_resource);
    if (!buffer) {
        wl_client_post_implementation_error(wl_resource_get_client(surface->resource),
                               "Mansion frame bridge requires wl_shm buffers");
        return false;
    }
    const int width = wl_shm_buffer_get_width(buffer), height = wl_shm_buffer_get_height(buffer);
    const int stride = wl_shm_buffer_get_stride(buffer);
    const uint32_t format = wl_shm_buffer_get_format(buffer);
    if (format != WL_SHM_FORMAT_ARGB8888 && format != WL_SHM_FORMAT_XRGB8888) {
        wl_client_post_implementation_error(wl_resource_get_client(surface->resource),
                               "Mansion frame bridge supports only ARGB8888/XRGB8888");
        return false;
    }
    // Bounds cover multiplication, the client stride and allocation. The total
    // cap counts frames retained by this compositor; caller-owned copies are separate.
    constexpr size_t max_frame_bytes = 64u * 1024 * 1024;
    constexpr size_t max_compositor_bytes = 256u * 1024 * 1024;
    if (width <= 0 || height <= 0 || int64_t(stride) < int64_t(width) * 4 ||
        uint64_t(width) * height > max_frame_bytes / 4) {
        wl_client_post_implementation_error(wl_resource_get_client(surface->resource),
                               "shm frame exceeds supported dimensions/64 MiB limit");
        return false;
    }
    const size_t bytes = size_t(width) * height * 4;
    const size_t retained = bytes + surface_tree_storage(surface->compositor);
    if (retained > max_compositor_bytes) {
        wl_client_post_implementation_error(wl_resource_get_client(surface->resource),
                               "compositor frame storage exceeds 256 MiB limit");
        return false;
    }
    frame.width = width; frame.height = height; frame.stride = width * 4;
    frame.source_format = format; frame.source_stride = stride; frame.mapped = true;
    frame.pixels.resize(bytes);
    wl_shm_buffer_begin_access(buffer);
    const auto* source = static_cast<const uint8_t*>(wl_shm_buffer_get_data(buffer));
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            uint32_t pixel;
            std::memcpy(&pixel, source + size_t(y) * stride + size_t(x) * 4, 4);
            uint32_t alpha = format == WL_SHM_FORMAT_XRGB8888 ? 255 : pixel >> 24;
            auto* target = frame.pixels.data() + (size_t(y) * width + x) * 4;
            for (int c = 0; c < 3; ++c) {
                const uint32_t premultiplied = (pixel >> (16 - 8 * c)) & 255;
                target[c] = alpha ? std::min(255u, (premultiplied * 255 + alpha / 2) / alpha) : 0;
            }
            target[3] = alpha;
        }
    }
    wl_shm_buffer_end_access(buffer);
    return true;
}

static void surface_commit(struct wl_client* client, struct wl_resource* resource) {
    auto* surface = static_cast<MansionSurface*>(wl_resource_get_user_data(resource));
    auto& state = *surface->core_frame;
    if (surface->xdg_surface && !xdg_shell_on_surface_commit(surface->xdg_surface)) return;
    const bool metadata_changed = state.scale != state.pending_scale || state.transform != state.pending_transform;
    if (state.pending_attach || (metadata_changed && state.frame)) {
        try {
            auto next = state.pending_attach ? std::make_shared<mansion::FrameSnapshot>()
                                             : std::make_shared<mansion::FrameSnapshot>(*state.frame);
            if (state.pending_attach && surface->pending_buffer_resource && !copy_frame(surface, *next)) return;
            next->scale = state.pending_scale; next->transform = state.pending_transform;
            const bool rotated = next->transform & 1;
            const int transformed_width = rotated ? next->height : next->width;
            const int transformed_height = rotated ? next->width : next->height;
            if (transformed_width % next->scale || transformed_height % next->scale) {
                wl_resource_post_error(resource, WL_SURFACE_ERROR_INVALID_SIZE,
                                       "buffer dimensions must be divisible by buffer scale");
                return;
            }
            next->logical_width = transformed_width / next->scale;
            next->logical_height = transformed_height / next->scale;
            next->handle = state.handle; next->revision = ++state.revision;
            state.frame = std::move(next);
        } catch (const std::bad_alloc&) {
            wl_client_post_no_memory(client);
            return;
        }
        surface->width = state.frame->logical_width;
        surface->height = state.frame->logical_height;
        if (state.pending_attach && !surface->pending_buffer_resource && surface->xdg_surface)
            xdg_shell_on_surface_unmap(surface->xdg_surface);
    }
    state.scale = state.pending_scale; state.transform = state.pending_transform;
    if (state.pending_attach && surface->pending_buffer_resource) {
        // Copy is complete. Never keep or read this client buffer after release.
        wl_buffer_send_release(surface->pending_buffer_resource);
    }
    wl_list_remove(&surface->buffer_destroy_listener.link);
    wl_list_init(&surface->buffer_destroy_listener.link);
    surface->pending_buffer_resource = nullptr;
    surface->buffer_resource = nullptr;
    state.pending_attach = false;
    if (surface->has_pending_position) {
        surface->current_x = surface->pending_x; surface->current_y = surface->pending_y;
        surface->has_current_position = true; surface->has_pending_position = false;
    }
    surface->damage_count = 0;
    try { surface_tree_commit(surface); }
    catch (const std::bad_alloc&) { wl_client_post_no_memory(client); return; }
    if (surface->xdg_surface) {
        try {
            const auto bounds = surface_tree_bounds(surface);
            xdg_shell_on_surface_applied(surface->xdg_surface, bounds.width, bounds.height, bounds.x, bounds.y);
        } catch (const std::bad_alloc&) { wl_client_post_no_memory(client); return; }
    }
    if (surface->commit_callback) surface->commit_callback(resource, surface->commit_callback_user_data);
}

static void surface_frame(struct wl_client* client, struct wl_resource* resource,
                          uint32_t callback_id) {
    (void)client;
    auto* surface = static_cast<MansionSurface*>(wl_resource_get_user_data(resource));

    auto* cb = wl_resource_create(client, &wl_callback_interface, 1, callback_id);
    if (!cb) {
        wl_client_post_no_memory(client);
        return;
    }
    wl_resource_set_destructor(cb, frame_callback_destroyed);
    wl_list_insert(surface->frame_callback_list.prev, wl_resource_get_link(cb));
}

static void surface_set_opaque_region(struct wl_client* client, struct wl_resource* resource,
                                      struct wl_resource* region) {
    (void)client; (void)resource; (void)region;
}

static void surface_set_input_region(struct wl_client* client, struct wl_resource* resource,
                                      struct wl_resource* region) {
    auto* surface = static_cast<MansionSurface*>(wl_resource_get_user_data(resource));
    try {
        surface->core_frame->pending_input = region
            ? InputRegion(*static_cast<std::vector<InputRectangle>*>(wl_resource_get_user_data(region))) : std::nullopt;
    } catch (const std::bad_alloc&) { wl_client_post_no_memory(client); }
}

static void surface_set_buffer_transform(struct wl_client* client, struct wl_resource* resource,
                                         int32_t transform) {
    (void)client;
    if (transform < 0 || transform > 7) {
        wl_resource_post_error(resource, WL_SURFACE_ERROR_INVALID_TRANSFORM, "invalid buffer transform");
        return;
    }
    auto* surface = static_cast<MansionSurface*>(wl_resource_get_user_data(resource));
    surface->core_frame->pending_transform = transform;
}

static void surface_set_buffer_scale(struct wl_client* client, struct wl_resource* resource,
                                     int32_t scale) {
    (void)client;
    if (scale <= 0) {
        wl_resource_post_error(resource, WL_SURFACE_ERROR_INVALID_SCALE, "invalid buffer scale");
        return;
    }
    auto* surface = static_cast<MansionSurface*>(wl_resource_get_user_data(resource));
    surface->core_frame->pending_scale = scale;
}

static void surface_damage_buffer(struct wl_client* client, struct wl_resource* resource,
                                  int32_t x, int32_t y, int32_t width, int32_t height) {
    surface_damage(client, resource, x, y, width, height);
}

static const struct wl_surface_interface surface_impl = {
    surface_destroy,
    surface_attach,
    surface_damage,
    surface_frame,
    surface_set_opaque_region,
    surface_set_input_region,
    surface_commit,
    surface_set_buffer_transform,
    surface_set_buffer_scale,
    surface_damage_buffer,
    nullptr, /* offset (v5) */
    nullptr, /* get_release (v7) */
};

/* ---------- wl_region ---------- */

static void region_destroy(struct wl_client* client, struct wl_resource* resource) {
    (void)client;
    wl_resource_destroy(resource);
}

static void region_change(wl_client* client, wl_resource* resource, int32_t x, int32_t y, int32_t width, int32_t height, bool subtract) {
    if (width <= 0 || height <= 0) return;
    auto& region = *static_cast<std::vector<InputRectangle>*>(wl_resource_get_user_data(resource));
    if (region.size() >= 1024) { wl_client_post_implementation_error(client, "input region exceeds 1024 operations"); return; }
    try { region.push_back({x, y, width, height, subtract}); }
    catch (const std::bad_alloc&) { wl_client_post_no_memory(client); }
}
static void region_add(wl_client* c, wl_resource* r, int32_t x, int32_t y, int32_t w, int32_t h) { region_change(c,r,x,y,w,h,false); }
static void region_subtract(wl_client* c, wl_resource* r, int32_t x, int32_t y, int32_t w, int32_t h) { region_change(c,r,x,y,w,h,true); }
static void region_destroyed(wl_resource* r) { delete static_cast<std::vector<InputRectangle>*>(wl_resource_get_user_data(r)); }

static const struct wl_region_interface region_impl = {
    region_destroy,
    region_add,
    region_subtract,
};

/* ---------- compositor ---------- */

static bool initialize_frame_state(MansionSurface* surface, wl_client* client) {
    static std::atomic<int64_t> next_handle{1};
    surface->core_frame = new (std::nothrow) CoreFrameState;
    if (!surface->core_frame) { wl_client_post_no_memory(client); return false; }
    // Saturate instead of allowing an old handle to become valid again.
    int64_t handle = next_handle.load();
    do {
        if (handle == std::numeric_limits<int64_t>::max()) {
            delete surface->core_frame; surface->core_frame = nullptr;
            wl_client_post_implementation_error(client, "runtime surface handles exhausted");
            return false;
        }
    } while (!next_handle.compare_exchange_weak(handle, handle + 1));
    surface->core_frame->handle = handle;
    wl_list_init(&surface->core_frame->committed_callbacks);
    wl_list_init(&surface->core_frame->cached_callbacks);
    return true;
}

static void compositor_create_surface(struct wl_client* client, struct wl_resource* compositor_resource,
                                       uint32_t id) {
    auto* compositor = static_cast<MansionCompositor*>(wl_resource_get_user_data(compositor_resource));

    auto* surface = new (std::nothrow) MansionSurface{};
    if (!surface) {
        wl_client_post_no_memory(client);
        return;
    }
    if (!initialize_frame_state(surface, client)) { delete surface; return; }
    surface->resource = wl_resource_create(client, &wl_surface_interface, wl_resource_get_version(compositor_resource), id);
    if (!surface->resource) {
        delete surface->core_frame;
        delete surface;
        wl_client_post_no_memory(client);
        return;
    }

    wl_resource_set_implementation(surface->resource, &surface_impl, surface, surface_destroy_callback);

    wl_list_init(&surface->link);
    wl_list_insert(&compositor->surface_list, &surface->link);

    surface->compositor = compositor;

    surface->xdg_surface = nullptr;
    surface->buffer_resource = nullptr;
    surface->pending_buffer_resource = nullptr;
    wl_list_init(&surface->buffer_destroy_listener.link);
    surface->buffer_destroyed = false;
    /* GL texture is now owned by the renderer layer */
    surface->has_pending_position = false;
    surface->has_current_position = false;
    surface->width = 0;
    surface->height = 0;
    surface->needs_upload = false;
    surface->damage_count = 0;
    surface->pending_x_damage = 0;
    surface->pending_y_damage = 0;
    surface->pending_w_damage = 0;
    surface->pending_h_damage = 0;
    wl_list_init(&surface->frame_callback_list);
    surface->commit_callback = nullptr;
    surface->commit_callback_user_data = nullptr;
}

static void compositor_create_region(struct wl_client* client, struct wl_resource* compositor_resource,
                                       uint32_t id) {
    auto* compositor = static_cast<MansionCompositor*>(wl_resource_get_user_data(compositor_resource));
    (void)compositor;

    auto* region = wl_resource_create(client, &wl_region_interface, 1, id);
    if (!region) {
        wl_client_post_no_memory(client);
        return;
    }
    auto* rectangles = new (std::nothrow) std::vector<InputRectangle>;
    if (!rectangles) { wl_resource_destroy(region); wl_client_post_no_memory(client); return; }
    wl_resource_set_implementation(region, &region_impl, rectangles, region_destroyed);
}

static void compositor_release(struct wl_client* client, struct wl_resource* resource) {
    (void)client;
    wl_resource_destroy(resource);
}

static const struct wl_compositor_interface compositor_impl = {
    compositor_create_surface,
    compositor_create_region,
    compositor_release,
};

static void compositor_bind(struct wl_client* client, void* data, uint32_t version, uint32_t id) {
    auto* compositor = static_cast<MansionCompositor*>(data);
    auto* resource = wl_resource_create(client, &wl_compositor_interface, std::min(version, 4u), id);
    if (!resource) {
        wl_client_post_no_memory(client);
        return;
    }
    wl_resource_set_implementation(resource, &compositor_impl, compositor, nullptr);
}

/* ---------- Core API implementation ---------- */

struct MansionCompositor* compositor_core_create(struct wl_display* display) {
    if (!display) return nullptr;
    auto* compositor = new (std::nothrow) MansionCompositor{};
    if (!compositor) return nullptr;
    wl_list_init(&compositor->surface_list);
    wl_list_init(&compositor->orphaned_surfaces);
    wl_list_init(&compositor->toplevel_list);
    compositor->toplevel_count = 0;
    compositor->focused_surface_resource = nullptr;
    compositor->keyboard_focus_serial = 0;
    compositor->seat = nullptr;

    compositor->global = wl_global_create(display, &wl_compositor_interface, 4,
                                           compositor, compositor_bind);
    if (!compositor->global) {
        delete compositor;
        return nullptr;
    }

    return compositor;
}

void compositor_core_destroy(struct MansionCompositor* compositor) {
    if (!compositor) return;
    wl_global_destroy(compositor->global);

    struct MansionSurface *surface, *next;
    wl_list_for_each_safe(surface, next, &compositor->surface_list, link) {
        wl_resource_destroy(surface->resource);
    }

    /* Clean up orphaned surfaces (surfaces that survived client disconnect). */
    wl_list_for_each_safe(surface, next, &compositor->orphaned_surfaces, link) {
        wl_list_remove(&surface->link);
        delete surface;
    }

    delete compositor;
}

struct wl_display* compositor_core_get_display(struct MansionCompositor* compositor) {
    if (!compositor) return nullptr;
    return wl_global_get_display(compositor->global);
}

struct wl_list* compositor_core_get_surface_list(struct MansionCompositor* compositor) {
    if (!compositor) return nullptr;
    return &compositor->surface_list;
}

struct wl_list* compositor_core_get_toplevel_list(struct MansionCompositor* compositor) {
    if (!compositor) return nullptr;
    return &compositor->toplevel_list;
}

int compositor_core_get_toplevel_count(struct MansionCompositor* compositor) {
    if (!compositor) return 0;
    return compositor->toplevel_count;
}

struct wl_resource* compositor_core_get_focused_surface(struct MansionCompositor* compositor) {
    if (!compositor) return nullptr;
    return compositor->focused_surface_resource;
}

uint32_t compositor_core_get_focus_serial(struct MansionCompositor* compositor) {
    if (!compositor) return 0;
    return compositor->keyboard_focus_serial;
}

void compositor_core_set_focus(struct MansionCompositor* compositor,
                               struct wl_resource* surface,
                               uint32_t serial) {
    if (!compositor) return;
    compositor->focused_surface_resource = surface;
    compositor->keyboard_focus_serial = serial;
}

void compositor_core_clear_focus(struct MansionCompositor* compositor) {
    if (!compositor) return;
    compositor->focused_surface_resource = nullptr;
    compositor->keyboard_focus_serial = 0;
}

void compositor_core_connect_seat(struct MansionCompositor* compositor,
                                   struct MansionSeat* seat) {
    if (!compositor) return;
    compositor->seat = seat;
}

struct wl_resource* compositor_core_surface_first(struct wl_list* surface_list) {
    if (!surface_list || wl_list_empty(surface_list)) return nullptr;
    MansionSurface* surface = wl_container_of(surface_list->next, surface, link);
    return surface->resource;
}

struct wl_resource* compositor_core_surface_next(struct wl_resource* resource) {
    if (!resource) return nullptr;
    auto* surface = static_cast<MansionSurface*>(wl_resource_get_user_data(resource));
    if (!surface || surface->link.next == &surface->compositor->surface_list) return nullptr;
    MansionSurface* next = wl_container_of(surface->link.next, next, link);
    return next->resource;
}

void* compositor_core_surface_get_user_data(struct wl_resource* resource) {
    return wl_resource_get_user_data(resource);
}

void compositor_core_surface_set_user_data(struct wl_resource* resource,
                                           void* user_data) {
    wl_resource_set_user_data(resource, user_data);
}

struct MansionSurface* compositor_surface_from_serial(struct MansionCompositor* compositor, uint32_t serial) {
    if (!compositor) return nullptr;

    struct MansionSurface *surface, *next;
    wl_list_for_each_safe(surface, next, &compositor->surface_list, link) {
        if (surface->client_serial == serial) {
            return surface;
        }
    }

    /* Also check orphaned surfaces */
    wl_list_for_each_safe(surface, next, &compositor->orphaned_surfaces, link) {
        if (surface->client_serial == serial) {
            return surface;
        }
    }

    return nullptr;
}

struct wl_resource* compositor_core_create_surface(struct MansionCompositor* compositor,
                                                   struct wl_client* client,
                                                   uint32_t id) {
    if (!compositor || !client) return nullptr;

    auto* surface = new (std::nothrow) MansionSurface{};
    if (!surface) {
        wl_client_post_no_memory(client);
        return nullptr;
    }
    if (!initialize_frame_state(surface, client)) { delete surface; return nullptr; }
    surface->resource = wl_resource_create(client, &wl_surface_interface, 4, id);
    if (!surface->resource) {
        delete surface->core_frame;
        delete surface;
        wl_client_post_no_memory(client);
        return nullptr;
    }

    wl_resource_set_implementation(surface->resource, &surface_impl, surface, surface_destroy_callback);

    wl_list_init(&surface->link);
    wl_list_insert(&compositor->surface_list, &surface->link);

    surface->compositor = compositor;

    surface->xdg_surface = nullptr;
    surface->buffer_resource = nullptr;
    surface->pending_buffer_resource = nullptr;
    wl_list_init(&surface->buffer_destroy_listener.link);
    surface->buffer_destroyed = false;
    surface->has_pending_position = false;
    surface->has_current_position = false;
    surface->width = 0;
    surface->height = 0;
    surface->needs_upload = false;
    surface->damage_count = 0;
    surface->pending_x_damage = 0;
    surface->pending_y_damage = 0;
    surface->pending_w_damage = 0;
    surface->pending_h_damage = 0;
    surface->client_serial = 0;  /* Will be set by caller */
    wl_list_init(&surface->frame_callback_list);
    surface->commit_callback = nullptr;
    surface->commit_callback_user_data = nullptr;

    return surface->resource;
}

void compositor_core_destroy_surface(struct wl_resource* surface_resource) {
    if (!surface_resource) return;
    wl_resource_destroy(surface_resource);
}

int compositor_core_surface_list_empty(struct MansionCompositor* compositor) {
    if (!compositor) return 1;
    return wl_list_empty(&compositor->surface_list);
}

int compositor_core_orphaned_surfaces_empty(struct MansionCompositor* compositor) {
    if (!compositor) return 1;
    return wl_list_empty(&compositor->orphaned_surfaces);
}

int compositor_core_toplevel_list_empty(struct MansionCompositor* compositor) {
    if (!compositor) return 1;
    return wl_list_empty(&compositor->toplevel_list);
}

void compositor_core_surface_set_commit_callback(struct wl_resource* surface_resource,
                                                  surface_commit_callback callback,
                                                  void* user_data) {
    auto* surface = static_cast<MansionSurface*>(wl_resource_get_user_data(surface_resource));
    if (surface) {
        surface->commit_callback = callback;
        surface->commit_callback_user_data = user_data;
    }
}

void compositor_core_complete_frames(MansionCompositor* compositor) {
    if (!compositor) return;
    const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    MansionSurface* surface;
    wl_list_for_each(surface, &compositor->surface_list, link) {
        auto& callbacks = surface->core_frame->committed_callbacks;
        while (!wl_list_empty(&callbacks)) {
            auto* callback = wl_resource_from_link(callbacks.next);
            wl_callback_send_done(callback, static_cast<uint32_t>(now));
            wl_resource_destroy(callback);
        }
    }
}
