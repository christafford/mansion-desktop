#include <gdextension_interface.h>
#include <cstdio>

/* GDExtension interface function pointers */
static GDExtensionInterfaceGetGodotVersion get_godot_version = nullptr;
static GDExtensionInterfaceClassdbRegisterExtensionClass classdb_register_extension_class = nullptr;
static GDExtensionInterfaceClassdbRegisterExtensionClass6 classdb_register_extension_class6 = nullptr;

static void init_interface_functions(GDExtensionInterfaceGetProcAddress p_get_proc_address) {
    get_godot_version = (GDExtensionInterfaceGetGodotVersion)p_get_proc_address("get_godot_version");
    classdb_register_extension_class = (GDExtensionInterfaceClassdbRegisterExtensionClass)p_get_proc_address("classdb_register_extension_class");
    classdb_register_extension_class6 = (GDExtensionInterfaceClassdbRegisterExtensionClass6)p_get_proc_address("classdb_register_extension_class6");
}

/* Minimal GDExtension initialization - just returns success without registering anything */
static GDExtensionBool minimal_extension_init(
    GDExtensionInterfaceGetProcAddress p_get_proc_address,
    GDExtensionClassLibraryPtr p_library,
    GDExtensionInitialization *r_initialization) {
    
    fprintf(stderr, "[minimal] minimal_extension_init called\n");
    fflush(stderr);
    
    init_interface_functions(p_get_proc_address);
    
    /* Just set the minimum initialization level and return */
    r_initialization->minimum_initialization_level = GDEXTENSION_INITIALIZATION_CORE;
    r_initialization->userdata = nullptr;
    r_initialization->initialize = nullptr;
    r_initialization->deinitialize = nullptr;
    
    fprintf(stderr, "[minimal] Initialization complete\n");
    fflush(stderr);
    
    return 1; /* GDEXTENSION_BOOL_TRUE */
}

static void minimal_extension_destroy(GDExtensionClassLibraryPtr p_library) {
    fprintf(stderr, "[minimal] Extension destroyed\n");
    fflush(stderr);
}

/* Entry point called by Godot loader */
extern "C" {
    GDExtensionBool GDExtensionInit(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
        fprintf(stderr, "[minimal] GDExtensionInit called\n");
        fflush(stderr);
        
        GDExtensionBool result = minimal_extension_init(p_get_proc_address, p_library, r_initialization);
        
        fprintf(stderr, "[minimal] GDExtensionInit returning %d\n", result);
        fflush(stderr);
        
        return result;
    }
    
    void GDExtensionExtensionDestroy(GDExtensionClassLibraryPtr p_library) {
        fprintf(stderr, "[minimal] Extension destroyed\n");
        fflush(stderr);
    }
}
