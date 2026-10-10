#include "compositor_session.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <cstring>

namespace elsewhere {
void ElsewhereCompositorSession::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("request_resize", "handle", "width", "height"), &ElsewhereCompositorSession::request_resize);
    godot::ClassDB::bind_method(godot::D_METHOD("window_state", "handle"), &ElsewhereCompositorSession::window_state);
    godot::ClassDB::bind_method(godot::D_METHOD("pointer_motion", "handle", "x", "y"), &ElsewhereCompositorSession::pointer_motion);
    godot::ClassDB::bind_method(godot::D_METHOD("pointer_button", "button", "pressed"), &ElsewhereCompositorSession::pointer_button);
    godot::ClassDB::bind_method(godot::D_METHOD("pointer_axis", "horizontal", "vertical"), &ElsewhereCompositorSession::pointer_axis);
    godot::ClassDB::bind_method(godot::D_METHOD("pointer_reset"), &ElsewhereCompositorSession::pointer_reset);
    godot::ClassDB::bind_method(godot::D_METHOD("pointer_focus_handle"), &ElsewhereCompositorSession::pointer_focus_handle);
    godot::ClassDB::bind_method(godot::D_METHOD("pointer_grabbed"), &ElsewhereCompositorSession::pointer_grabbed);
    godot::ClassDB::bind_method(godot::D_METHOD("focus_keyboard", "handle"), &ElsewhereCompositorSession::focus_keyboard);
    godot::ClassDB::bind_method(godot::D_METHOD("keyboard_focus_handle"), &ElsewhereCompositorSession::keyboard_focus_handle);
    godot::ClassDB::bind_method(godot::D_METHOD("keyboard_key", "evdev_code", "pressed"), &ElsewhereCompositorSession::keyboard_key);
    godot::ClassDB::bind_method(godot::D_METHOD("surface_handles"), &ElsewhereCompositorSession::surface_handles);
    godot::ClassDB::bind_method(godot::D_METHOD("toplevel_handles"), &ElsewhereCompositorSession::toplevel_handles);
    godot::ClassDB::bind_method(godot::D_METHOD("snapshot", "handle", "after_revision"), &ElsewhereCompositorSession::snapshot, DEFVAL(0));
    godot::ClassDB::bind_method(godot::D_METHOD("start", "runtime_directory"), &ElsewhereCompositorSession::start);
    godot::ClassDB::bind_method(godot::D_METHOD("pump"), &ElsewhereCompositorSession::pump);
    godot::ClassDB::bind_method(godot::D_METHOD("stop"), &ElsewhereCompositorSession::stop);
    godot::ClassDB::bind_method(godot::D_METHOD("is_running"), &ElsewhereCompositorSession::is_running);
    godot::ClassDB::bind_method(godot::D_METHOD("client_count"), &ElsewhereCompositorSession::client_count);
    godot::ClassDB::bind_method(godot::D_METHOD("toplevel_count"), &ElsewhereCompositorSession::toplevel_count);
    godot::ClassDB::bind_method(godot::D_METHOD("surface_count"), &ElsewhereCompositorSession::surface_count);
    godot::ClassDB::bind_method(godot::D_METHOD("socket_path"), &ElsewhereCompositorSession::socket_path);
    godot::ClassDB::bind_method(godot::D_METHOD("last_error"), &ElsewhereCompositorSession::last_error);
}

godot::PackedInt64Array ElsewhereCompositorSession::surface_handles() const {
    godot::PackedInt64Array handles;
    for (auto handle : runtime_.surface_handles()) handles.push_back(handle);
    return handles;
}

godot::PackedInt64Array ElsewhereCompositorSession::toplevel_handles() const {
    godot::PackedInt64Array handles;
    for (auto handle : runtime_.toplevel_handles()) handles.push_back(handle);
    return handles;
}

godot::Dictionary ElsewhereCompositorSession::snapshot(int64_t handle, int64_t after_revision) const {
    auto frame = runtime_.snapshot(handle);
    godot::Dictionary result;
    if (!frame || (after_revision >= 0 && frame->revision <= static_cast<uint64_t>(after_revision))) return result;
    result["handle"] = frame->handle;
    result["revision"] = static_cast<int64_t>(frame->revision);
    result["mapped"] = frame->mapped;
    result["width"] = frame->width; result["height"] = frame->height;
    result["stride"] = frame->stride;
    result["logical_width"] = frame->logical_width; result["logical_height"] = frame->logical_height;
    result["origin_x"] = frame->origin_x; result["origin_y"] = frame->origin_y;
    result["scale"] = frame->scale; result["transform"] = frame->transform;
    result["source_format"] = frame->source_format; result["source_stride"] = frame->source_stride;
    result["format"] = "RGBA8";
    godot::PackedByteArray pixels;
    if (pixels.resize(frame->pixels.size()) != godot::OK) {
        ERR_PRINT("Could not allocate Godot frame snapshot");
        return {};
    }
    if (!frame->pixels.empty()) std::memcpy(pixels.ptrw(), frame->pixels.data(), frame->pixels.size());
    result["pixels"] = pixels;
    return result;
}

godot::Dictionary ElsewhereCompositorSession::window_state(int64_t handle) const {
    godot::Dictionary result;
    auto state = runtime_.window_state(handle);
    if (!state) return result;
    result["declared_width"] = state->declared_width; result["declared_height"] = state->declared_height;
    result["surface_width"] = state->surface_width; result["surface_height"] = state->surface_height;
    result["x"] = state->x; result["y"] = state->y;
    result["width"] = state->width; result["height"] = state->height;
    result["min_width"] = state->min_width; result["min_height"] = state->min_height;
    result["max_width"] = state->max_width; result["max_height"] = state->max_height;
    result["requested_width"] = state->requested_width; result["requested_height"] = state->requested_height;
    result["sent_serial"] = state->sent_serial; result["acked_serial"] = state->acked_serial;
    result["committed_serial"] = state->committed_serial;
    return result;
}

bool ElsewhereCompositorSession::start(const godot::String& runtime_directory) {
    return runtime_.start(runtime_directory.utf8().get_data());
}
} // namespace elsewhere
