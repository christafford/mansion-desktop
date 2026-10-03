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
