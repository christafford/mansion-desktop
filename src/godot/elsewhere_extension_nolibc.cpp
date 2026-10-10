#include <gdextension_interface.h>

/* Minimal GDExtension entry point - no external dependencies */
__attribute__((visibility("default")))
GDExtensionBool GDExtensionInit(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
    return 1;
}

__attribute__((visibility("default")))
void GDExtensionExtensionDestroy(GDExtensionClassLibraryPtr p_library) {
}
