#include "compositor-runtime.h"
#include "compositor-core.h"
#include "compositor-private.h"
#include "core-frame-state.h"
#include "surface-tree.h"
#include "seat.h"
#include "xdg-shell.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <vector>
#include <unordered_set>
#include <new>
#include <wayland-server-protocol.h>
#include <wayland-server-core.h>

// One logical display for the nested workspace, independent of the host's
// monitor registry. Bound output objects belong to their clients.
struct ElsewhereOutput {
    wl_global* global = nullptr;
    ElsewhereCompositor* compositor = nullptr;
    wl_list resources;
};
namespace {
struct OutputBinding {
    wl_resource* resource;
    ElsewhereOutput* output;
    std::unordered_set<int64_t> entered;
};
void update_output(OutputBinding* binding) try {
    std::unordered_set<int64_t> live;
    ElsewhereSurface* surface;
    wl_list_for_each(surface, &binding->output->compositor->surface_list, link) {
        if (wl_resource_get_client(surface->resource) != wl_resource_get_client(binding->resource)) continue;
        const auto handle = surface->core_frame->handle;
        live.insert(handle);
        const bool mapped = surface_tree_mapped(surface);
        if (mapped && binding->entered.insert(handle).second)
            wl_surface_send_enter(surface->resource, binding->resource);
        else if (!mapped && binding->entered.erase(handle))
            wl_surface_send_leave(surface->resource, binding->resource);
    }
    // Destroyed surfaces no longer have a protocol object to send leave to.
    std::erase_if(binding->entered, [&](int64_t handle) { return !live.contains(handle); });
}
catch (const std::bad_alloc&) {
    wl_client_post_no_memory(wl_resource_get_client(binding->resource));
}
void release_output(wl_client*, wl_resource* resource) { wl_resource_destroy(resource); }
const struct wl_output_interface output_impl = {release_output};
void output_destroyed(wl_resource* resource) {
    wl_list_remove(wl_resource_get_link(resource));
    delete static_cast<OutputBinding*>(wl_resource_get_user_data(resource));
}
void bind_output(wl_client* client, void* data, uint32_t version, uint32_t id) {
    auto* output = static_cast<ElsewhereOutput*>(data);
    auto* resource = wl_resource_create(client, &wl_output_interface, std::min(version, 3u), id);
    if (!resource) { wl_client_post_no_memory(client); return; }
    auto* binding = new (std::nothrow) OutputBinding{resource, output, {}};
    if (!binding) { wl_resource_destroy(resource); wl_client_post_no_memory(client); return; }
    wl_resource_set_implementation(resource, &output_impl, binding, output_destroyed);
    wl_list_insert(&output->resources, wl_resource_get_link(resource));
    wl_output_send_geometry(resource, 0, 0, 338, 211, WL_OUTPUT_SUBPIXEL_UNKNOWN,
                            "Elsewhere", "Workspace", WL_OUTPUT_TRANSFORM_NORMAL);
    wl_output_send_mode(resource, WL_OUTPUT_MODE_CURRENT | WL_OUTPUT_MODE_PREFERRED, 1280, 800, 60000);
    if (version >= 2) { wl_output_send_scale(resource, 1); wl_output_send_done(resource); }
    update_output(binding);
}
}

namespace elsewhere {
CompositorRuntime::~CompositorRuntime() {
    if (!stop()) std::fprintf(stderr, "Elsewhere server cleanup: %s\n", error_.c_str());
}

bool CompositorRuntime::fail(const std::string& operation) {
    error_ = operation + ": " + std::strerror(errno);
    return false;
}

bool CompositorRuntime::start(const std::string& runtime_directory) {
    if (display_ || !directory_.empty()) {
        error_ = "server already started or previous cleanup incomplete";
        return false;
    }
    error_.clear();
    if (runtime_directory.empty() || runtime_directory[0] != '/') {
        error_ = "runtime directory must be an absolute existing directory";
        return false;
    }
    std::string pattern = runtime_directory + "/elsewhere-XXXXXX";
    std::vector<char> name(pattern.begin(), pattern.end());
    name.push_back('\0');
    if (!mkdtemp(name.data())) return fail("create private runtime directory");
    directory_ = name.data();
    socket_path_ = directory_ + "/wayland";
    display_ = wl_display_create();
    if (!display_) {
        fail("create Wayland display");
        stop();
        return false;
    }
    if (wl_display_init_shm(display_) < 0) {
        fail("initialize shm global");
        stop();
        return false;
    }
    compositor_ = compositor_core_create(display_);
    if (!compositor_) {
        fail("create compositor global");
        stop();
        return false;
    }
    seat_ = seat_create(display_, 5);
    shell_ = create_xdg_shell(compositor_, display_);
    if (!seat_ || !shell_) {
        fail("create seat/xdg-shell globals");
        stop();
        return false;
    }
    compositor_set_seat(compositor_, seat_);
    subcompositor_ = surface_tree_create_global(display_);
    if (!subcompositor_) { error_ = "create subcompositor global"; stop(); return false; }
    output_ = new (std::nothrow) ElsewhereOutput{};
    if (!output_) { error_ = "allocate workspace output"; stop(); return false; }
    output_->compositor = compositor_;
    wl_list_init(&output_->resources);
    output_->global = wl_global_create(display_, &wl_output_interface, 3, output_, bind_output);
    if (!output_->global) { error_ = "create workspace output global"; stop(); return false; }
    // An absolute name avoids changing the host XDG_RUNTIME_DIR/WAYLAND_DISPLAY.
    if (wl_display_add_socket(display_, socket_path_.c_str()) < 0) {
        fail("bind private Wayland socket");
        stop();
        return false;
    }
    return true;
}

bool CompositorRuntime::pump() {
    if (!display_) {
        error_ = "cannot pump a stopped server";
        return false;
    }
    // Server API, zero polling timeout. Never dispatch a client wl_display here.
    if (wl_event_loop_dispatch(wl_display_get_event_loop(display_), 0) < 0)
        return fail("dispatch Wayland server loop");
    wl_resource* output_resource;
    wl_resource_for_each(output_resource, &output_->resources)
        update_output(static_cast<OutputBinding*>(wl_resource_get_user_data(output_resource)));
    compositor_core_complete_frames(compositor_);
    if (compositor_->focused_surface_resource && keyboard_focus_handle() == 0)
        seat_set_keyboard_focus(seat_, nullptr, compositor_);
    if (seat_pointer_surface(seat_) && pointer_focus_handle() == 0) seat_pointer_reset(seat_);
    wl_display_flush_clients(display_);
    return true;
}

bool CompositorRuntime::stop() {
    if (display_) {
        // Resource callbacks still need the compositor during client teardown.
        wl_display_destroy_clients(display_);
        if (output_) {
            if (output_->global) wl_global_destroy(output_->global);
            delete output_;
            output_ = nullptr;
        }
        if (subcompositor_) wl_global_destroy(subcompositor_);
        subcompositor_ = nullptr;
        destroy_xdg_shell(shell_);
        shell_ = nullptr;
        seat_destroy(seat_);
        seat_ = nullptr;
        compositor_core_destroy(compositor_);
        compositor_ = nullptr;
        wl_display_destroy(display_);
        display_ = nullptr;
    }
    if (!directory_.empty()) {
        // libwayland removed its socket and lock; remove only our empty directory.
        if (rmdir(directory_.c_str()) < 0) return fail("remove private runtime directory");
        directory_.clear();
    }
    socket_path_.clear();
    return true;
}

std::vector<int64_t> CompositorRuntime::surface_handles() const {
    std::vector<int64_t> result;
    if (!compositor_) return result;
    ElsewhereSurface* surface;
    wl_list_for_each(surface, &compositor_->surface_list, link)
        result.push_back(surface->core_frame->handle);
    return result;
}

std::vector<int64_t> CompositorRuntime::toplevel_handles() const {
    std::vector<int64_t> result;
    if (!compositor_) return result;
    ElsewhereSurface* surface;
    wl_list_for_each(surface, &compositor_->surface_list, link)
        if (xdg_surface_has_toplevel(surface->xdg_surface)) result.push_back(surface->core_frame->handle);
    return result;
}

OwnedFrame CompositorRuntime::snapshot(int64_t handle) const {
    if (!compositor_ || handle <= 0) return {};
    ElsewhereSurface* surface;
    wl_list_for_each(surface, &compositor_->surface_list, link)
        if (surface->core_frame->handle == handle) return surface_tree_snapshot(surface);
    return {};
}

std::optional<XdgWindowState> CompositorRuntime::window_state(int64_t handle) const {
    if (!compositor_) return {};
    ElsewhereSurface* surface;
    wl_list_for_each(surface, &compositor_->surface_list, link) {
        const auto& frame = surface->core_frame->frame;
        if (surface->core_frame->handle == handle && frame && frame->mapped &&
            xdg_surface_has_toplevel(surface->xdg_surface)) {
            auto result = xdg_surface_window_state(surface->xdg_surface);
            result.surface_width = frame->logical_width; result.surface_height = frame->logical_height;
            if (auto composed = surface_tree_snapshot(surface)) {
                if (result.geometry_is_set) {
                    result.x -= composed->origin_x; result.y -= composed->origin_y;
                } else {
                    // Default geometry follows even independently committed
                    // child content; explicit geometry stays fixed by xdg-shell.
                    result.x = result.y = 0;
                    result.width = result.declared_width = composed->logical_width;
                    result.height = result.declared_height = composed->logical_height;
                }
            }
            return result;
        }
    }
    return {};
}

bool CompositorRuntime::request_resize(int64_t handle, int32_t width, int32_t height) {
    if (compositor_) {
        ElsewhereSurface* surface;
        wl_list_for_each(surface, &compositor_->surface_list, link) {
            const auto& frame = surface->core_frame->frame;
            if (surface->core_frame->handle == handle && frame && frame->mapped &&
                xdg_shell_request_resize(surface->xdg_surface, width, height)) return true;
        }
    }
    error_ = "resize requires a live mapped toplevel, dimensions/limits within 1..2048 and fewer than 64 pending configures";
    return false;
}

int64_t CompositorRuntime::keyboard_focus_handle() const {
    if (!compositor_ || !compositor_->focused_surface_resource) return 0;
    ElsewhereSurface* surface;
    wl_list_for_each(surface, &compositor_->surface_list, link) {
        const auto& frame = surface->core_frame->frame;
        if (surface->resource == compositor_->focused_surface_resource && frame &&
            frame->mapped && xdg_surface_has_toplevel(surface->xdg_surface))
            return surface->core_frame->handle;
    }
    return 0;
}

bool CompositorRuntime::focus_keyboard(int64_t handle) {
    if (!compositor_) { error_ = "cannot focus a stopped server"; return false; }
    if (handle == 0) {
        seat_set_keyboard_focus(seat_, nullptr, compositor_);
        return true;
    }
    ElsewhereSurface* surface;
    wl_list_for_each(surface, &compositor_->surface_list, link) {
        const auto& frame = surface->core_frame->frame;
        if (surface->core_frame->handle == handle && frame && frame->mapped &&
            xdg_surface_has_toplevel(surface->xdg_surface)) {
            seat_set_keyboard_focus(seat_, surface->resource, compositor_);
            return true;
        }
    }
    error_ = "keyboard focus requires a live mapped toplevel";
    return false;
}

bool CompositorRuntime::keyboard_key(uint32_t evdev_code, bool pressed) {
    if (!keyboard_focus_handle()) {
        error_ = "keyboard input requires a live focused toplevel";
        return false;
    }
    if (!seat_keyboard_key(seat_, evdev_code, pressed)) {
        error_ = "invalid Linux evdev keycode";
        return false;
    }
    return true;
}

int64_t CompositorRuntime::pointer_focus_handle() const {
    if (!compositor_) return 0;
    ElsewhereSurface* surface;
    wl_list_for_each(surface, &compositor_->surface_list, link) {
        if (surface->resource != seat_pointer_surface(seat_) || !surface_tree_mapped(surface)) continue;
        auto* root = surface_tree_root(surface);
        if (root && xdg_surface_has_toplevel(root->xdg_surface)) return root->core_frame->handle;
    }
    return 0;
}

bool CompositorRuntime::pointer_motion(int64_t handle, double x, double y) try {
    if (!compositor_) { error_ = "pointer input requires a running server"; return false; }
    if (handle == 0) {
        if (seat_pointer_motion(seat_, nullptr, x, y)) return true;
        error_ = "pointer leave requires valid coordinates and no active grab; use reset to cancel";
        return false;
    }
    ElsewhereSurface* root;
    wl_list_for_each(root, &compositor_->surface_list, link) {
        if (root->core_frame->handle != handle || !surface_tree_mapped(root) || !xdg_surface_has_toplevel(root->xdg_surface)) continue;
        auto frame = surface_tree_snapshot(root);
        if (!frame) break;
        if (!pointer_grabbed() && (x < 0 || y < 0 || x >= frame->logical_width || y >= frame->logical_height)) break;
        x += frame->origin_x; y += frame->origin_y;
        ElsewhereSurface* target = nullptr;
        if (pointer_grabbed()) {
            target = static_cast<ElsewhereSurface*>(wl_resource_get_user_data(seat_pointer_surface(seat_)));
            if (!surface_tree_coordinates(root, target, x, y)) break;
        } else target = surface_tree_at(root, x, y);
        if (seat_pointer_motion(seat_, target ? target->resource : nullptr, x, y)) return true;
        break;
    }
    error_ = "pointer motion requires a live mapped target, valid coordinates and matching grab";
    return false;
} catch (const std::bad_alloc&) { error_ = "allocate surface-tree hit test"; return false; }

bool CompositorRuntime::pointer_button(uint32_t button, bool pressed) {
    if (pointer_focus_handle() && seat_pointer_button(seat_, button, pressed)) return true;
    error_ = "pointer button requires a live focused toplevel and supported evdev button";
    return false;
}

bool CompositorRuntime::pointer_axis(double horizontal, double vertical) {
    if (pointer_focus_handle() && seat_pointer_axis(seat_, horizontal, vertical)) return true;
    error_ = "pointer scroll requires a live focused toplevel and finite bounded values";
    return false;
}

void CompositorRuntime::pointer_reset() { seat_pointer_reset(seat_); }
bool CompositorRuntime::pointer_grabbed() const { return seat_pointer_grabbed(seat_); }

int CompositorRuntime::client_count() const {
    return display_ ? wl_list_length(wl_display_get_client_list(display_)) : 0;
}

int CompositorRuntime::toplevel_count() const {
    return compositor_ ? compositor_->toplevel_count : 0;
}

int CompositorRuntime::surface_count() const {
    if (!compositor_) return 0;
    int count = 0;
    for (auto* resource = compositor_core_surface_first(compositor_core_get_surface_list(compositor_));
         resource; resource = compositor_core_surface_next(resource)) ++count;
    return count;
}
} // namespace elsewhere
