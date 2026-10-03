#include "compositor_session.h"
#include <godot_cpp/core/class_db.hpp>

namespace mansion {
void MansionCompositorSession::_bind_methods() {
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

bool MansionCompositorSession::start(const godot::String& runtime_directory) {
    return runtime_.start(runtime_directory.utf8().get_data());
}
} // namespace mansion
