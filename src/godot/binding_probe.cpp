// Small load/call/lifetime regression for the standard godot-cpp boundary.
// The separate MansionCompositorSession class owns the lifecycle under test.
#include "compositor_session.h"
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/godot.hpp>

namespace mansion {
class MansionBindingProbe : public godot::RefCounted {
    GDCLASS(MansionBindingProbe, godot::RefCounted)

protected:
    static void _bind_methods() {
        godot::ClassDB::bind_method(godot::D_METHOD("answer"),
                                   &MansionBindingProbe::answer);
    }

public:
    int64_t answer() const { return 42; }
};

void initialize(godot::ModuleInitializationLevel level) {
    if (level == godot::MODULE_INITIALIZATION_LEVEL_SCENE) {
        GDREGISTER_CLASS(MansionBindingProbe);
        GDREGISTER_CLASS(MansionCompositorSession);
    }
}

void uninitialize(godot::ModuleInitializationLevel) {}
} // namespace mansion

extern "C" GDExtensionBool GDE_EXPORT mansion_probe_init(
    GDExtensionInterfaceGetProcAddress get_proc_address,
    GDExtensionClassLibraryPtr library, GDExtensionInitialization *initialization) {
    godot::GDExtensionBinding::InitObject init(get_proc_address, library, initialization);
    init.register_initializer(mansion::initialize);
    init.register_terminator(mansion::uninitialize);
    init.set_minimum_library_initialization_level(godot::MODULE_INITIALIZATION_LEVEL_SCENE);
    return init.init();
}
