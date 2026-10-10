#pragma once

#include <string>
#include <optional>
#include "xdg-shell.h"
#include "frame-snapshot.h"

struct wl_display;
struct wl_global;
struct ElsewhereCompositor;
struct ElsewhereSeat;
struct ElsewhereXdgShell;
struct ElsewhereOutput;

namespace elsewhere {
// Single-threaded server ownership. All calls and destruction belong to the
// creating thread, driven by Godot's frame loop or the headless test harness.
// Owns compositor/shm, xdg-shell and seat protocol state, independent of rendering.
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
    int toplevel_count() const;
    std::vector<int64_t> surface_handles() const;
    std::vector<int64_t> toplevel_handles() const;
    OwnedFrame snapshot(int64_t handle) const;
    bool request_resize(int64_t handle, int32_t width, int32_t height);
    std::optional<XdgWindowState> window_state(int64_t handle) const;
    bool focus_keyboard(int64_t handle); // 0 returns to world/no client focus.
    int64_t keyboard_focus_handle() const;
    bool keyboard_key(uint32_t evdev_code, bool pressed);
    bool pointer_motion(int64_t handle, double x, double y); // 0 leaves (unless grabbed).
    bool pointer_button(uint32_t button, bool pressed);
    bool pointer_axis(double horizontal, double vertical);
    void pointer_reset();
    int64_t pointer_focus_handle() const;
    bool pointer_grabbed() const;
    const std::string& socket_path() const { return socket_path_; }
    const std::string& last_error() const { return error_; }
    bool running() const { return display_ != nullptr; }

private:
    bool fail(const std::string& operation);
    wl_display* display_ = nullptr;
    ElsewhereCompositor* compositor_ = nullptr;
    ElsewhereSeat* seat_ = nullptr;
    ElsewhereXdgShell* shell_ = nullptr;
    ElsewhereOutput* output_ = nullptr;
    wl_global* subcompositor_ = nullptr;
    std::string directory_;
    std::string socket_path_;
    std::string error_;
};
} // namespace elsewhere
