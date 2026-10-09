#include "surface-tree.h"
#include "core-frame-state.h"
#include "compositor-private.h"
#include <wayland-server-protocol.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <new>

struct MansionSubsurface {
    wl_resource* resource = nullptr;
    MansionSurface* surface = nullptr;
    MansionSurface* parent = nullptr;
    int32_t x = 0, y = 0, pending_x = 0, pending_y = 0, cached_x = 0, cached_y = 0;
    bool sync = true;
};
namespace {
auto& state(MansionSurface* s) { return *s->core_frame; }
MansionSurface* parent(MansionSurface* s) {
    return state(s).subsurface ? state(s).subsurface->parent : nullptr;
}
bool synchronized(MansionSurface* s) {
    for (; s && state(s).subsurface; s = parent(s))
        if (state(s).subsurface->sync) return true;
    return false;
}
void apply(MansionSurface* s, bool ancestor_sync) {
    auto& f = state(s);
    if (f.cached_commit) {
        f.visible_frame = f.frame;
        f.visible_input = f.cached_input;
        wl_list_insert_list(f.committed_callbacks.prev, &f.cached_callbacks);
        wl_list_init(&f.cached_callbacks);
        f.cached_commit = false;
        f.order = f.cached_order;
        for (auto* child : f.order) if (child != s) {
            auto* sub = state(child).subsurface;
            sub->x = sub->cached_x; sub->y = sub->cached_y;
        }
    }
    for (auto* child : f.order) if (child != s) {
        const bool sync = ancestor_sync || state(child).subsurface->sync;
        if (sync) apply(child, true);
    }
}
void detach(MansionSubsurface* sub) {
    if (sub->parent) {
        auto& p = state(sub->parent);
        std::erase(p.order, sub->surface);
        std::erase(p.pending_order, sub->surface);
        std::erase(p.cached_order, sub->surface);
        sub->parent = nullptr;
    }
}
void destroy_request(wl_client*, wl_resource* r) { wl_resource_destroy(r); }
void sub_destroyed(wl_resource* r) {
    auto* sub = static_cast<MansionSubsurface*>(wl_resource_get_user_data(r));
    detach(sub);
    if (sub->surface) {
        state(sub->surface).subsurface = nullptr;
        state(sub->surface).visible_frame.reset();
    }
    delete sub;
}
void position(wl_client*, wl_resource* r, int32_t x, int32_t y) {
    // Bound eventual composite allocation and integer arithmetic explicitly.
    if (x < -8192 || x > 8192 || y < -8192 || y > 8192) {
        wl_client_post_implementation_error(wl_resource_get_client(r), "subsurface position exceeds Mansion's +/-8192 limit");
        return;
    }
    auto* sub = static_cast<MansionSubsurface*>(wl_resource_get_user_data(r));
    sub->pending_x = x; sub->pending_y = y;
}
void place(wl_resource* r, wl_resource* sibling_resource, bool above) {
    auto* sub = static_cast<MansionSubsurface*>(wl_resource_get_user_data(r));
    if (!sub->surface || !sub->parent) return;
    auto* sibling = static_cast<MansionSurface*>(wl_resource_get_user_data(sibling_resource));
    auto& order = state(sub->parent).pending_order;
    if (sibling == sub->surface || std::find(order.begin(), order.end(), sibling) == order.end()) {
        wl_resource_post_error(r, WL_SUBSURFACE_ERROR_BAD_SURFACE, "stacking target must be parent or sibling");
        return;
    }
    std::erase(order, sub->surface);
    auto it = std::find(order.begin(), order.end(), sibling);
    // Erase retained vector capacity, so this insert cannot allocate.
    order.insert(it + (above ? 1 : 0), sub->surface);
}
void above(wl_client*, wl_resource* r, wl_resource* sibling) { place(r, sibling, true); }
void below(wl_client*, wl_resource* r, wl_resource* sibling) { place(r, sibling, false); }
void sync(wl_client*, wl_resource* r) { static_cast<MansionSubsurface*>(wl_resource_get_user_data(r))->sync = true; }
void desync(wl_client* client, wl_resource* r) {
    auto* sub = static_cast<MansionSubsurface*>(wl_resource_get_user_data(r));
    sub->sync = false;
    if (sub->surface && !synchronized(sub->surface)) {
        try { apply(sub->surface, false); }
        catch (const std::bad_alloc&) { wl_client_post_no_memory(client); }
    }
}
const struct wl_subsurface_interface sub_impl = {destroy_request, position, above, below, sync, desync};
int subtree_depth(MansionSurface* s) {
    int depth = 1;
    for (auto* child : state(s).pending_order) if (child != s) depth = std::max(depth, 1 + subtree_depth(child));
    return depth;
}
void get_subsurface(wl_client* client, wl_resource* r, uint32_t id, wl_resource* child_r, wl_resource* parent_r) try {
    auto* child = static_cast<MansionSurface*>(wl_resource_get_user_data(child_r));
    auto* p = static_cast<MansionSurface*>(wl_resource_get_user_data(parent_r));
    if (!child || !p || !child->core_frame || !p->core_frame || child->xdg_surface ||
        state(child).subsurface) {
        wl_resource_post_error(r, WL_SUBCOMPOSITOR_ERROR_BAD_SURFACE, "surface already has a role or invalid parent"); return;
    }
    int depth = 0;
    for (auto* ancestor = p; ancestor; ancestor = parent(ancestor)) {
        if (ancestor == child || ++depth > 64) {
            wl_resource_post_error(r, WL_SUBCOMPOSITOR_ERROR_BAD_PARENT, "cyclic or excessive subsurface hierarchy"); return;
        }
    }
    if (depth + subtree_depth(child) > 64) {
        wl_resource_post_error(r, WL_SUBCOMPOSITOR_ERROR_BAD_PARENT, "subsurface hierarchy exceeds 64 levels"); return;
    }
    auto& pf = state(p);
    if (pf.pending_order.size() >= 256) {
        wl_client_post_implementation_error(client, "Mansion supports at most 255 direct subsurfaces"); return;
    }
    if (pf.pending_order.empty()) { pf.pending_order.push_back(p); pf.order.push_back(p); }
    // Allocate before creating a live resource with cross-links.
    pf.pending_order.reserve(pf.pending_order.size() + 1);
    auto* sub = new (std::nothrow) MansionSubsurface;
    if (!sub) { wl_client_post_no_memory(client); return; }
    sub->resource = wl_resource_create(client, &wl_subsurface_interface, 1, id);
    if (!sub->resource) { delete sub; wl_client_post_no_memory(client); return; }
    sub->surface = child; sub->parent = p;
    state(child).subsurface = sub;
    state(child).subsurface_role = true;
    state(child).visible_frame.reset();
    pf.pending_order.push_back(child);
    for (auto* ancestor = p; ancestor; ancestor = parent(ancestor)) state(ancestor).tree_used = true;
    wl_resource_set_implementation(sub->resource, &sub_impl, sub, sub_destroyed);
} catch (const std::bad_alloc&) { wl_client_post_no_memory(client); }
const struct wl_subcompositor_interface impl = {destroy_request, get_subsurface};
void bind(wl_client* client, void*, uint32_t, uint32_t id) {
    auto* r = wl_resource_create(client, &wl_subcompositor_interface, 1, id);
    if (!r) { wl_client_post_no_memory(client); return; }
    wl_resource_set_implementation(r, &impl, nullptr, nullptr);
}
struct Layer { MansionSurface* surface; int x, y; };
void collect(MansionSurface* s, int x, int y, std::vector<Layer>& layers) {
    const auto& f = state(s);
    if (!f.visible_frame || !f.visible_frame->mapped) return;
    if (f.order.empty()) { layers.push_back({s, x, y}); return; }
    for (auto* child : f.order) {
        if (child == s) layers.push_back({s, x, y});
        else {
            auto* sub = state(child).subsurface;
            collect(child, x + sub->x, y + sub->y, layers);
        }
    }
}
bool input_contains(const InputRegion& region, double x, double y) {
    if (!region) return true;
    bool inside = false;
    for (const auto& rect : *region)
        if (x >= rect.x && y >= rect.y && x < int64_t(rect.x) + rect.width && y < int64_t(rect.y) + rect.height)
            inside = !rect.subtract;
    return inside;
}
const uint8_t* pixel(const mansion::FrameSnapshot& f, int x, int y) {
    const int rw = (f.transform & 1) ? f.height : f.width;
    if (f.transform >= 4) x = rw - 1 - x;
    int sx = x, sy = y;
    switch (f.transform % 4) {
    case 1: sx = y; sy = f.height - 1 - x; break;
    case 2: sx = f.width - 1 - x; sy = f.height - 1 - y; break;
    case 3: sx = f.width - 1 - y; sy = x; break;
    }
    return &f.pixels[(size_t(sy) * f.width + sx) * 4];
}
} // namespace

wl_global* surface_tree_create_global(wl_display* display) {
    return wl_global_create(display, &wl_subcompositor_interface, 1, nullptr, bind);
}
void surface_tree_commit(MansionSurface* s) {
    auto& f = state(s);
    f.cached_input = f.pending_input;
    f.cached_order = f.pending_order;
    for (auto* child : f.cached_order) if (child != s) {
        auto* sub = state(child).subsurface;
        sub->cached_x = sub->pending_x; sub->cached_y = sub->pending_y;
    }
    f.cached_commit = true;
    wl_list_insert_list(f.cached_callbacks.prev, &s->frame_callback_list);
    wl_list_init(&s->frame_callback_list);
    if (!synchronized(s)) apply(s, false);
}
void surface_tree_destroyed(MansionSurface* s) {
    auto& f = state(s);
    if (f.subsurface) { detach(f.subsurface); f.subsurface->surface = nullptr; }
    for (auto* child : f.pending_order) if (child != s) state(child).subsurface->parent = nullptr;
    for (auto* child : f.order) if (child != s) state(child).subsurface->parent = nullptr;
    while (!wl_list_empty(&f.cached_callbacks)) wl_resource_destroy(wl_resource_from_link(f.cached_callbacks.next));
}
MansionSurface* surface_tree_root(MansionSurface* s) {
    while (s && state(s).subsurface_role) s = parent(s);
    return s;
}
bool surface_tree_mapped(MansionSurface* s) {
    for (; s; s = parent(s)) {
        if (!state(s).visible_frame || !state(s).visible_frame->mapped) return false;
        if (state(s).subsurface_role && !parent(s)) return false;
        if (parent(s)) {
            const auto& order = state(parent(s)).order;
            if (std::find(order.begin(), order.end(), s) == order.end()) return false;
        }
    }
    return true;
}
mansion::OwnedFrame surface_tree_snapshot(MansionSurface* s) try {
    auto& f = state(s);
    if (!f.tree_used || !f.visible_frame) return f.visible_frame;
    std::vector<Layer> layers;
    collect(s, 0, 0, layers);
    std::vector<int64_t> signature{static_cast<int64_t>(f.visible_frame->revision)};
    int left = 0, top = 0, right = f.visible_frame->logical_width, bottom = f.visible_frame->logical_height;
    for (auto layer : layers) {
        const auto& image = *state(layer.surface).visible_frame;
        signature.insert(signature.end(), {image.handle, static_cast<int64_t>(image.revision), layer.x, layer.y});
        left = std::min(left, layer.x); top = std::min(top, layer.y);
        right = std::max(right, layer.x + image.logical_width); bottom = std::max(bottom, layer.y + image.logical_height);
    }
    if (f.composite && f.composite_signature == signature) return f.composite;
    const int scale = f.visible_frame->scale;
    const int64_t width = int64_t(right - left) * scale, height = int64_t(bottom - top) * scale;
    if (width > 4096 || height > 4096 || width * height > 16777216) {
        wl_client_post_implementation_error(wl_resource_get_client(s->resource), "composed surface exceeds Mansion's 4096px/64 MiB limit"); return {};
    }
    if (surface_tree_storage(s->compositor) + size_t(width * height * 4) > 256u * 1024 * 1024) {
        wl_client_post_implementation_error(wl_resource_get_client(s->resource), "composed frame storage exceeds 256 MiB limit"); return {};
    }
    auto result = std::make_shared<mansion::FrameSnapshot>();
    result->handle = f.handle;
    f.composite_revision = std::max(f.composite_revision + 1, f.visible_frame->revision);
    result->revision = f.composite_revision;
    result->mapped = f.visible_frame->mapped;
    result->logical_width = right - left; result->logical_height = bottom - top;
    result->width = width; result->height = height; result->stride = width * 4;
    result->scale = scale; result->origin_x = left; result->origin_y = top;
    result->pixels.resize(size_t(width * height * 4));
    bool empty_canvas = true;
    for (auto layer : layers) {
        const auto& image = *state(layer.surface).visible_frame;
        // The bottom layer replaces transparent canvas. Most frames are an
        // upright root alone, so avoid millions of scalar alpha divisions.
        if (empty_canvas && image.transform == 0 && image.scale == scale) {
            for (int y = 0; y < image.height; ++y) {
                auto* dst = &result->pixels[(size_t((layer.y - top) * scale + y) * width + (layer.x - left) * scale) * 4];
                std::memcpy(dst, &image.pixels[size_t(y) * image.stride], size_t(image.width) * 4);
            }
            empty_canvas = false;
            continue;
        }
        empty_canvas = false;
        for (int y = 0; y < image.logical_height * scale; ++y) for (int x = 0; x < image.logical_width * scale; ++x) {
            const auto* src = pixel(image, x * image.scale / scale, y * image.scale / scale);
            auto* dst = &result->pixels[(size_t((layer.y - top) * scale + y) * width + (layer.x - left) * scale + x) * 4];
            if (src[3] == 0) continue;
            if (src[3] == 255 || dst[3] == 0) { std::memcpy(dst, src, 4); continue; }
            const unsigned alpha = src[3] * 255 + dst[3] * (255 - src[3]);
            for (int c = 0; c < 3; ++c)
                dst[c] = alpha ? (src[c] * src[3] * 255 + dst[c] * dst[3] * (255 - src[3]) + alpha / 2) / alpha : 0;
            dst[3] = (alpha + 127) / 255;
        }
    }
    f.composite_signature = std::move(signature);
    f.composite = result;
    return result;
} catch (const std::bad_alloc&) { wl_client_post_no_memory(wl_resource_get_client(s->resource)); return {}; }
MansionSurface* surface_tree_at(MansionSurface* root, double& x, double& y) {
    std::vector<Layer> layers;
    collect(root, 0, 0, layers);
    for (auto it = layers.rbegin(); it != layers.rend(); ++it) {
        const auto& f = state(it->surface);
        const double sx = x - it->x, sy = y - it->y;
        if (sx >= 0 && sy >= 0 && sx < f.visible_frame->logical_width && sy < f.visible_frame->logical_height && input_contains(f.visible_input, sx, sy)) {
            x = sx; y = sy; return it->surface;
        }
    }
    return nullptr;
}
bool surface_tree_coordinates(MansionSurface* root, MansionSurface* target, double& x, double& y) {
    if (surface_tree_root(target) != root || !surface_tree_mapped(target)) return false;
    for (auto* s = target; s != root; s = parent(s)) { x -= state(s).subsurface->x; y -= state(s).subsurface->y; }
    return true;
}

size_t surface_tree_storage(MansionCompositor* compositor) {
    size_t bytes = 0;
    MansionSurface* s;
    wl_list_for_each(s, &compositor->surface_list, link) {
        const auto& f = state(s);
        if (f.frame) bytes += f.frame->pixels.size();
        if (f.visible_frame && f.visible_frame != f.frame) bytes += f.visible_frame->pixels.size();
        if (f.composite && f.composite != f.frame && f.composite != f.visible_frame) bytes += f.composite->pixels.size();
    }
    return bytes;
}

SurfaceTreeBounds surface_tree_bounds(MansionSurface* surface) {
    std::vector<Layer> layers;
    collect(surface, 0, 0, layers);
    int left = 0, top = 0, right = surface->width, bottom = surface->height;
    for (auto layer : layers) {
        const auto& f = *state(layer.surface).visible_frame;
        left = std::min(left, layer.x); top = std::min(top, layer.y);
        right = std::max(right, layer.x + f.logical_width); bottom = std::max(bottom, layer.y + f.logical_height);
    }
    return {left, top, right - left, bottom - top};
}
