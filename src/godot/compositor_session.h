#pragma once
#include "../compositor-runtime.h"
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/string.hpp>

namespace mansion {
// Godot owns this on its main thread. Native runtime owns every server resource.
class MansionCompositorSession : public godot::RefCounted {
    GDCLASS(MansionCompositorSession, godot::RefCounted)
    CompositorRuntime runtime_;

protected:
    static void _bind_methods();

public:
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
} // namespace mansion
