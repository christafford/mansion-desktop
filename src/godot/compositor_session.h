#pragma once
#include "../compositor-runtime.h"
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_int64_array.hpp>

namespace elsewhere {
// Godot owns this on its main thread. Native runtime owns every server resource.
class ElsewhereCompositorSession : public godot::RefCounted {
    GDCLASS(ElsewhereCompositorSession, godot::RefCounted)
    CompositorRuntime runtime_;

protected:
    static void _bind_methods();

public:
    godot::PackedInt64Array surface_handles() const;
    godot::PackedInt64Array toplevel_handles() const;
    godot::Dictionary snapshot(int64_t handle, int64_t after_revision = 0) const;
    godot::Dictionary window_state(int64_t handle) const;
    bool request_resize(int64_t handle, int64_t width, int64_t height) {
        return width >= 1 && width <= 2048 && height >= 1 && height <= 2048 &&
            runtime_.request_resize(handle, static_cast<int32_t>(width), static_cast<int32_t>(height));
    }
    bool focus_keyboard(int64_t handle) { return runtime_.focus_keyboard(handle); }
    int64_t keyboard_focus_handle() const { return runtime_.keyboard_focus_handle(); }
    bool keyboard_key(int64_t evdev_code, bool pressed) {
        return evdev_code > 0 && evdev_code <= 767 && runtime_.keyboard_key(static_cast<uint32_t>(evdev_code), pressed);
    }
    bool pointer_motion(int64_t handle, double x, double y) { return runtime_.pointer_motion(handle, x, y); }
    bool pointer_button(int64_t button, bool pressed) {
        return button >= 272 && button <= 279 && runtime_.pointer_button(static_cast<uint32_t>(button), pressed);
    }
    bool pointer_axis(double horizontal, double vertical) { return runtime_.pointer_axis(horizontal, vertical); }
    void pointer_reset() { runtime_.pointer_reset(); }
    int64_t pointer_focus_handle() const { return runtime_.pointer_focus_handle(); }
    bool pointer_grabbed() const { return runtime_.pointer_grabbed(); }
    bool start(const godot::String& runtime_directory);
    bool pump() { return runtime_.pump(); }
    bool stop() { return runtime_.stop(); }
    bool is_running() const { return runtime_.running(); }
    int64_t client_count() const { return runtime_.client_count(); }
    int64_t toplevel_count() const { return runtime_.toplevel_count(); }
    int64_t surface_count() const { return runtime_.surface_count(); }
    godot::String socket_path() const { return godot::String::utf8(runtime_.socket_path().c_str()); }
    godot::String last_error() const { return godot::String::utf8(runtime_.last_error().c_str()); }
};
} // namespace elsewhere
