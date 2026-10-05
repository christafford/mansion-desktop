#include <gdextension_interface.h>

/* Empty GDExtension that does nothing - just returns success */
extern "C" {

__attribute__((visibility("default")))
GDExtensionBool GDExtensionInit(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
    return 1; /* GDEXTENSION_BOOL_TRUE */
}

__attribute__((visibility("default")))
void GDExtensionExtensionDestroy(GDExtensionClassLibraryPtr p_library) {
}

}
