#include "compositor-runtime.h"
#include "compositor-core.h"
#include "compositor-private.h"
#include "core-frame-state.h"
#include "seat.h"
#include "xdg-shell.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <vector>
#include <wayland-server-core.h>

namespace mansion {
CompositorRuntime::~CompositorRuntime() {
    if (!stop()) std::fprintf(stderr, "Mansion server cleanup: %s\n", error_.c_str());
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
    std::string pattern = runtime_directory + "/mansion-XXXXXX";
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
    seat_ = seat_create(display_);
    shell_ = create_xdg_shell(compositor_, display_);
    if (!seat_ || !shell_) {
        fail("create seat/xdg-shell globals");
        stop();
        return false;
    }
    compositor_set_seat(compositor_, seat_);
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
    MansionSurface* surface;
    wl_list_for_each(surface, &compositor_->surface_list, link)
        result.push_back(surface->core_frame->handle);
    return result;
}

std::vector<int64_t> CompositorRuntime::toplevel_handles() const {
    std::vector<int64_t> result;
    if (!compositor_) return result;
    MansionSurface* surface;
    wl_list_for_each(surface, &compositor_->surface_list, link)
        if (xdg_surface_has_toplevel(surface->xdg_surface)) result.push_back(surface->core_frame->handle);
    return result;
}

OwnedFrame CompositorRuntime::snapshot(int64_t handle) const {
    if (!compositor_ || handle <= 0) return {};
    MansionSurface* surface;
    wl_list_for_each(surface, &compositor_->surface_list, link)
        if (surface->core_frame->handle == handle) return surface->core_frame->frame;
    return {};
}

std::optional<XdgWindowState> CompositorRuntime::window_state(int64_t handle) const {
    if (!compositor_) return {};
    MansionSurface* surface;
    wl_list_for_each(surface, &compositor_->surface_list, link) {
        const auto& frame = surface->core_frame->frame;
        if (surface->core_frame->handle == handle && frame && frame->mapped &&
            xdg_surface_has_toplevel(surface->xdg_surface)) return xdg_surface_window_state(surface->xdg_surface);
    }
    return {};
}

bool CompositorRuntime::request_resize(int64_t handle, int32_t width, int32_t height) {
    if (compositor_) {
        MansionSurface* surface;
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
    MansionSurface* surface;
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
    MansionSurface* surface;
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
    MansionSurface* surface;
    wl_list_for_each(surface, &compositor_->surface_list, link) {
        const auto& frame = surface->core_frame->frame;
        if (surface->resource == seat_pointer_surface(seat_) && frame && frame->mapped &&
            xdg_surface_has_toplevel(surface->xdg_surface)) return surface->core_frame->handle;
    }
    return 0;
}

bool CompositorRuntime::pointer_motion(int64_t handle, double x, double y) {
    if (!compositor_) { error_ = "pointer input requires a running server"; return false; }
    if (handle == 0) {
        if (seat_pointer_motion(seat_, nullptr, x, y)) return true;
        error_ = "pointer leave requires valid coordinates and no active grab; use reset to cancel";
        return false;
    }
    MansionSurface* surface;
    wl_list_for_each(surface, &compositor_->surface_list, link) {
        const auto& frame = surface->core_frame->frame;
        if (surface->core_frame->handle != handle || !frame || !frame->mapped ||
            !xdg_surface_has_toplevel(surface->xdg_surface)) continue;
        // Only an existing grab may travel outside the surface bounds.
        if (!pointer_grabbed() && (x < 0 || y < 0 || x >= frame->logical_width || y >= frame->logical_height)) break;
        if (seat_pointer_motion(seat_, surface->resource, x, y)) return true;
        break;
    }
    error_ = "pointer motion requires a live mapped target, valid coordinates and matching grab";
    return false;
}

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
} // namespace mansion
