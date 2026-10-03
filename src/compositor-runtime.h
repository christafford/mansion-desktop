#pragma once

#include <string>

struct wl_display;
struct MansionCompositor;

namespace mansion {
// Single-threaded server ownership. All calls and destruction belong to the
// creating thread, driven by Godot's frame loop or the headless test harness.
// This owns only wl_compositor/shm for now; xdg-shell and seat are later gates.
class CompositorRuntime {
public:
    CompositorRuntime() = default;
    ~CompositorRuntime();
    CompositorRuntime(const CompositorRuntime&) = delete;
    CompositorRuntime& operator=(const CompositorRuntime&) = delete;

    bool start(const std::string& runtime_directory);
    bool pump();
    bool stop();
    int client_count() const;
    int surface_count() const;
    const std::string& socket_path() const { return socket_path_; }
    const std::string& last_error() const { return error_; }
    bool running() const { return display_ != nullptr; }

private:
    bool fail(const std::string& operation);
    wl_display* display_ = nullptr;
    MansionCompositor* compositor_ = nullptr;
    std::string directory_;
    std::string socket_path_;
    std::string error_;
};
} // namespace mansion
