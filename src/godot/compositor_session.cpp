#include "compositor_session.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <cstring>

namespace mansion {
void MansionCompositorSession::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("pointer_motion", "handle", "x", "y"), &MansionCompositorSession::pointer_motion);
    godot::ClassDB::bind_method(godot::D_METHOD("pointer_button", "button", "pressed"), &MansionCompositorSession::pointer_button);
    godot::ClassDB::bind_method(godot::D_METHOD("pointer_axis", "horizontal", "vertical"), &MansionCompositorSession::pointer_axis);
    godot::ClassDB::bind_method(godot::D_METHOD("pointer_reset"), &MansionCompositorSession::pointer_reset);
    godot::ClassDB::bind_method(godot::D_METHOD("pointer_focus_handle"), &MansionCompositorSession::pointer_focus_handle);
    godot::ClassDB::bind_method(godot::D_METHOD("pointer_grabbed"), &MansionCompositorSession::pointer_grabbed);
    godot::ClassDB::bind_method(godot::D_METHOD("focus_keyboard", "handle"), &MansionCompositorSession::focus_keyboard);
    godot::ClassDB::bind_method(godot::D_METHOD("keyboard_focus_handle"), &MansionCompositorSession::keyboard_focus_handle);
    godot::ClassDB::bind_method(godot::D_METHOD("keyboard_key", "evdev_code", "pressed"), &MansionCompositorSession::keyboard_key);
    godot::ClassDB::bind_method(godot::D_METHOD("surface_handles"), &MansionCompositorSession::surface_handles);
    godot::ClassDB::bind_method(godot::D_METHOD("toplevel_handles"), &MansionCompositorSession::toplevel_handles);
    godot::ClassDB::bind_method(godot::D_METHOD("snapshot", "handle", "after_revision"), &MansionCompositorSession::snapshot, DEFVAL(0));
    godot::ClassDB::bind_method(godot::D_METHOD("start", "runtime_directory"), &MansionCompositorSession::start);
    godot::ClassDB::bind_method(godot::D_METHOD("pump"), &MansionCompositorSession::pump);
    godot::ClassDB::bind_method(godot::D_METHOD("stop"), &MansionCompositorSession::stop);
    godot::ClassDB::bind_method(godot::D_METHOD("is_running"), &MansionCompositorSession::is_running);
    godot::ClassDB::bind_method(godot::D_METHOD("client_count"), &MansionCompositorSession::client_count);
    godot::ClassDB::bind_method(godot::D_METHOD("toplevel_count"), &MansionCompositorSession::toplevel_count);
    godot::ClassDB::bind_method(godot::D_METHOD("surface_count"), &MansionCompositorSession::surface_count);
    godot::ClassDB::bind_method(godot::D_METHOD("socket_path"), &MansionCompositorSession::socket_path);
    godot::ClassDB::bind_method(godot::D_METHOD("last_error"), &MansionCompositorSession::last_error);
}

godot::PackedInt64Array MansionCompositorSession::surface_handles() const {
    godot::PackedInt64Array handles;
    for (auto handle : runtime_.surface_handles()) handles.push_back(handle);
    return handles;
}

godot::PackedInt64Array MansionCompositorSession::toplevel_handles() const {
    godot::PackedInt64Array handles;
    for (auto handle : runtime_.toplevel_handles()) handles.push_back(handle);
    return handles;
}

godot::Dictionary MansionCompositorSession::snapshot(int64_t handle, int64_t after_revision) const {
    auto frame = runtime_.snapshot(handle);
    godot::Dictionary result;
    if (!frame || (after_revision >= 0 && frame->revision <= static_cast<uint64_t>(after_revision))) return result;
    result["handle"] = frame->handle;
    result["revision"] = static_cast<int64_t>(frame->revision);
    result["mapped"] = frame->mapped;
    result["width"] = frame->width; result["height"] = frame->height;
    result["stride"] = frame->stride;
    result["logical_width"] = frame->logical_width; result["logical_height"] = frame->logical_height;
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

bool MansionCompositorSession::start(const godot::String& runtime_directory) {
    return runtime_.start(runtime_directory.utf8().get_data());
}
} // namespace mansion
