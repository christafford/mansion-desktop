/*
 * MansionExtension — GDExtension entry point for Mansion Desktop.
 *
 * This file provides the GDExtension API symbols that Godot requires
 * to load and unload the extension. It uses the GDExtension C API
 * directly via function pointers obtained from p_get_proc_address.
 *
 * The extension creates a private Wayland compositor on init and
 * pumps events from the main thread. No rendering or input device
 * scanning is done by the extension — those are handled by the
 * Godot-side bridge in GDScript (P21-T12+).
 */

#include <gdextension_interface.h>

#include "mansion_gdextension_adapter.h"
#include "compositor-core.h"

#include <cstring>
#include <cstdio>
#include <cstdlib>

/* GDExtension interface function pointers */
static GDExtensionInterfaceGetProcAddress gdext_get_proc_address = nullptr;

/* Cached function pointers */
static GDExtensionInterfaceClassdbRegisterExtensionClass6 classdb_register_extension_class6 = nullptr;
static GDExtensionInterfaceClassdbRegisterExtensionClassMethod classdb_register_extension_class_method = nullptr;
static GDExtensionInterfaceStringNameNewWithLatin1Chars string_name_new_with_latin1_chars = nullptr;
static GDExtensionInterfaceStringNewWithLatin1Chars string_new_with_latin1_chars = nullptr;
static GDExtensionInterfaceStringNewWithLatin1CharsAndLen string_new_with_latin1_chars_and_len = nullptr;
static GDExtensionInterfaceVariantDestroy variant_destroy = nullptr;
static GDExtensionInterfaceVariantNewNil variant_new_nil = nullptr;
static GDExtensionInterfaceVariantConstruct variant_construct = nullptr;
static GDExtensionInterfaceVariantGetType variant_get_type = nullptr;
static GDExtensionInterfaceMemAlloc mem_alloc = nullptr;
static GDExtensionInterfaceMemFree mem_free = nullptr;

/* Get a function pointer from the GDExtension interface */
static void *get_interface_function(const char *name) {
    if (!gdext_get_proc_address) {
        return nullptr;
    }
    return (void *)gdext_get_proc_address(name);
}

/* Initialize GDExtension interface function pointers */
static void init_interface_functions(GDExtensionInterfaceGetProcAddress p_get_proc_address) {
    gdext_get_proc_address = p_get_proc_address;
    
    classdb_register_extension_class6 = (GDExtensionInterfaceClassdbRegisterExtensionClass6)
        get_interface_function("classdb_register_extension_class6");
    classdb_register_extension_class_method = (GDExtensionInterfaceClassdbRegisterExtensionClassMethod)
        get_interface_function("classdb_register_extension_class_method");
    string_name_new_with_latin1_chars = (GDExtensionInterfaceStringNameNewWithLatin1Chars)
        get_interface_function("string_name_new_with_latin1_chars");
    string_new_with_latin1_chars = (GDExtensionInterfaceStringNewWithLatin1Chars)
        get_interface_function("string_new_with_latin1_chars");
    string_new_with_latin1_chars_and_len = (GDExtensionInterfaceStringNewWithLatin1CharsAndLen)
        get_interface_function("string_new_with_latin1_chars_and_len");
    variant_destroy = (GDExtensionInterfaceVariantDestroy)
        get_interface_function("variant_destroy");
    variant_new_nil = (GDExtensionInterfaceVariantNewNil)
        get_interface_function("variant_new_nil");
    variant_construct = (GDExtensionInterfaceVariantConstruct)
        get_interface_function("variant_construct");
    variant_get_type = (GDExtensionInterfaceVariantGetType)
        get_interface_function("variant_get_type");
    mem_alloc = (GDExtensionInterfaceMemAlloc)
        get_interface_function("mem_alloc");
    mem_free = (GDExtensionInterfaceMemFree)
        get_interface_function("mem_free");
}

/* Opaque handle to our adapter instance */
typedef struct {
    MansionGDExtensionAdapter *adapter;
} MansionAdapterInstance;

/* Instance binding callbacks - minimal implementations matching GDExtension signatures */

/* GDExtensionClassSet - returns GDExtensionBool */
static GDExtensionBool mansion_adapter_set(
    GDExtensionClassInstancePtr p_instance,
    GDExtensionConstStringNamePtr p_name,
    GDExtensionConstVariantPtr p_value) {
    (void)p_instance;
    (void)p_name;
    (void)p_value;
    return false;
}

/* GDExtensionClassGet - returns GDExtensionBool */
static GDExtensionBool mansion_adapter_get(
    GDExtensionClassInstancePtr p_instance,
    GDExtensionConstStringNamePtr p_name,
    GDExtensionVariantPtr r_ret) {
    (void)p_instance;
    (void)p_name;
    (void)r_ret;
    return false;
}

/* GDExtensionClassGetPropertyList - returns const GDExtensionPropertyInfo* */
static const GDExtensionPropertyInfo *mansion_adapter_get_property_list(
    GDExtensionClassInstancePtr p_instance,
    uint32_t *r_count) {
    (void)p_instance;
    *r_count = 0;
    return nullptr;
}

/* GDExtensionClassFreePropertyList2 */
static void mansion_adapter_free_property_list(
    GDExtensionClassInstancePtr p_instance,
    const GDExtensionPropertyInfo *p_list,
    uint32_t p_count) {
    (void)p_instance;
    (void)p_list;
    (void)p_count;
}

/* GDExtensionClassPropertyCanRevert - returns GDExtensionBool */
static GDExtensionBool mansion_adapter_property_can_revert(
    GDExtensionClassInstancePtr p_instance,
    GDExtensionConstStringNamePtr p_name) {
    (void)p_instance;
    (void)p_name;
    return false;
}

/* GDExtensionClassPropertyGetRevert - returns GDExtensionBool */
static GDExtensionBool mansion_adapter_property_get_revert(
    GDExtensionClassInstancePtr p_instance,
    GDExtensionConstStringNamePtr p_name,
    GDExtensionVariantPtr r_ret) {
    (void)p_instance;
    (void)p_name;
    (void)r_ret;
    return false;
}

/* GDExtensionClassNotification2 */
static void mansion_adapter_notification(
    GDExtensionClassInstancePtr p_instance,
    int32_t p_what,
    GDExtensionBool p_reversed) {
    (void)p_instance;
    (void)p_what;
    (void)p_reversed;
}

/* GDExtensionClassToString */
static void mansion_adapter_to_string(
    GDExtensionClassInstancePtr p_instance,
    GDExtensionBool *r_is_valid,
    GDExtensionStringPtr p_out) {
    (void)p_instance;
    if (r_is_valid) {
        *r_is_valid = false;
    }
    if (p_out) {
        string_new_with_latin1_chars(p_out, "MansionAdapter");
    }
}

/* GDExtensionClassReference */
static void mansion_adapter_reference(GDExtensionClassInstancePtr p_instance) {
    (void)p_instance;
}

/* GDExtensionClassUnreference */
static void mansion_adapter_unreference(GDExtensionClassInstancePtr p_instance) {
    (void)p_instance;
}

/* GDExtensionClassCreateInstance3 */
static GDExtensionObjectPtr mansion_adapter_create_instance(
    void *p_class_userdata,
    GDExtensionBool p_notify_postinitialize) {
    (void)p_class_userdata;
    (void)p_notify_postinitialize;
    return nullptr;
}

/* GDExtensionClassFreeInstance */
static void mansion_adapter_free_instance(void *p_class_userdata, GDExtensionClassInstancePtr p_instance) {
    (void)p_class_userdata;
    (void)p_instance;
}

/* GDExtensionClassRecreateInstance */
static GDExtensionClassInstancePtr mansion_adapter_recreate_instance(
    void *p_class_userdata,
    GDExtensionObjectPtr p_object) {
    (void)p_class_userdata;
    (void)p_object;
    return nullptr;
}

/* Method: initialize(socket_name: String, debug: bool) -> bool */
static void mansion_adapter_initialize_call(
    void *method_userdata,
    GDExtensionClassInstancePtr p_instance,
    const GDExtensionConstVariantPtr *p_args,
    GDExtensionInt p_argument_count,
    GDExtensionVariantPtr r_return,
    GDExtensionCallError *r_error) {
    
    (void)method_userdata;
    (void)p_args;
    
    if (r_error) {
        r_error->error = GDEXTENSION_CALL_OK;
    }
    
    /* Validate argument count */
    if (p_argument_count != 2) {
        if (r_error) {
            r_error->error = GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS;
            r_error->argument = 2;
            r_error->expected = 2;
        }
        return;
    }
    
    MansionAdapterInstance *instance = (MansionAdapterInstance *)p_instance;
    
    if (!instance || !instance->adapter) {
        if (r_error) {
            r_error->error = GDEXTENSION_CALL_ERROR_INSTANCE_IS_NULL;
        }
        return;
    }
    
    /* The actual adapter is created by the GDScript side */
    /* This is just a placeholder for the method signature */
    
    /* Return true */
    GDExtensionBool result = true;
    memcpy(r_return, &result, sizeof(GDExtensionBool));
}

/* Method: shutdown() -> void */
static void mansion_adapter_shutdown_call(
    void *method_userdata,
    GDExtensionClassInstancePtr p_instance,
    const GDExtensionConstVariantPtr *p_args,
    GDExtensionInt p_argument_count,
    GDExtensionVariantPtr r_return,
    GDExtensionCallError *r_error) {
    
    (void)method_userdata;
    (void)p_args;
    
    if (r_error) {
        r_error->error = GDEXTENSION_CALL_OK;
    }
    
    if (p_argument_count != 0) {
        if (r_error) {
            r_error->error = GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS;
            r_error->argument = 0;
            r_error->expected = 0;
        }
        return;
    }
    
    MansionAdapterInstance *instance = (MansionAdapterInstance *)p_instance;
    
    if (instance && instance->adapter) {
        /* Note: adapter shutdown is handled by destroy() in GDScript */
        (void)instance;
    }
}

/* Method: destroy() -> void */
static void mansion_adapter_destroy_call(
    void *method_userdata,
    GDExtensionClassInstancePtr p_instance,
    const GDExtensionConstVariantPtr *p_args,
    GDExtensionInt p_argument_count,
    GDExtensionVariantPtr r_return,
    GDExtensionCallError *r_error) {
    
    (void)method_userdata;
    (void)p_args;
    (void)r_return;
    
    if (r_error) {
        r_error->error = GDEXTENSION_CALL_OK;
    }
    
    if (p_argument_count != 0) {
        if (r_error) {
            r_error->error = GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS;
            r_error->argument = 0;
            r_error->expected = 0;
        }
        return;
    }
    
    MansionAdapterInstance *instance = (MansionAdapterInstance *)p_instance;
    
    if (instance && instance->adapter) {
        mansion_gdextension_adapter_destroy(instance->adapter);
        instance->adapter = nullptr;
    }
}

/* Method: pump() -> int */
static void mansion_adapter_pump_call(
    void *method_userdata,
    GDExtensionClassInstancePtr p_instance,
    const GDExtensionConstVariantPtr *p_args,
    GDExtensionInt p_argument_count,
    GDExtensionVariantPtr r_return,
    GDExtensionCallError *r_error) {
    
    (void)method_userdata;
    (void)p_args;
    
    if (r_error) {
        r_error->error = GDEXTENSION_CALL_OK;
    }
    
    if (p_argument_count != 0) {
        if (r_error) {
            r_error->error = GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS;
            r_error->argument = 0;
            r_error->expected = 0;
        }
        return;
    }
    
    MansionAdapterInstance *instance = (MansionAdapterInstance *)p_instance;
    
    if (!instance || !instance->adapter) {
        GDExtensionInt result = 0;
        memcpy(r_return, &result, sizeof(GDExtensionInt));
        return;
    }
    
    int result = mansion_gdextension_adapter_pump(instance->adapter);
    GDExtensionInt result_val = result;
    memcpy(r_return, &result_val, sizeof(GDExtensionInt));
}

/* Method: flush() -> int */
static void mansion_adapter_flush_call(
    void *method_userdata,
    GDExtensionClassInstancePtr p_instance,
    const GDExtensionConstVariantPtr *p_args,
    GDExtensionInt p_argument_count,
    GDExtensionVariantPtr r_return,
    GDExtensionCallError *r_error) {
    
    (void)method_userdata;
    (void)p_args;
    
    if (r_error) {
        r_error->error = GDEXTENSION_CALL_OK;
    }
    
    if (p_argument_count != 0) {
        if (r_error) {
            r_error->error = GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS;
            r_error->argument = 0;
            r_error->expected = 0;
        }
        return;
    }
    
    MansionAdapterInstance *instance = (MansionAdapterInstance *)p_instance;
    
    if (!instance || !instance->adapter) {
        GDExtensionInt result = 0;
        memcpy(r_return, &result, sizeof(GDExtensionInt));
        return;
    }
    
    int result = mansion_gdextension_adapter_flush(instance->adapter);
    GDExtensionInt result_val = result;
    memcpy(r_return, &result_val, sizeof(GDExtensionInt));
}

/* Method: get_focused_serial() -> uint64 */
static void mansion_adapter_get_focused_serial_call(
    void *method_userdata,
    GDExtensionClassInstancePtr p_instance,
    const GDExtensionConstVariantPtr *p_args,
    GDExtensionInt p_argument_count,
    GDExtensionVariantPtr r_return,
    GDExtensionCallError *r_error) {
    
    (void)method_userdata;
    (void)p_args;
    
    if (r_error) {
        r_error->error = GDEXTENSION_CALL_OK;
    }
    
    if (p_argument_count != 0) {
        if (r_error) {
            r_error->error = GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS;
            r_error->argument = 0;
            r_error->expected = 0;
        }
        return;
    }
    
    MansionAdapterInstance *instance = (MansionAdapterInstance *)p_instance;
    
    if (!instance || !instance->adapter) {
        GDExtensionInt result = 0;
        memcpy(r_return, &result, sizeof(GDExtensionInt));
        return;
    }
    
    uint32_t result = mansion_gdextension_adapter_get_focused_serial(instance->adapter);
    GDExtensionInt result_val = result;
    memcpy(r_return, &result_val, sizeof(GDExtensionInt));
}

/* Class registration data */
static void mansion_register_class(GDExtensionClassLibraryPtr p_library) {
    static const char class_name[] = "MansionAdapter";
    static const char parent_class[] = "RefCounted";
    
    /* Create StringName for class name and parent class name */
    GDExtensionUninitializedStringNamePtr class_name_sn_ptr;
    GDExtensionUninitializedStringNamePtr parent_name_sn_ptr;
    
    /* Always initialize string names - the function is always available in Godot 4.5+ */
    string_name_new_with_latin1_chars(class_name_sn_ptr, class_name, false);
    string_name_new_with_latin1_chars(parent_name_sn_ptr, parent_class, false);
    
    /* Create StringName objects from the uninitialized pointers */
    GDExtensionStringNamePtr class_name_sn = (GDExtensionStringNamePtr)class_name_sn_ptr;
    GDExtensionStringNamePtr parent_name_sn = (GDExtensionStringNamePtr)parent_name_sn_ptr;
    
    /* Setup GDExtensionClassCreationInfo6 */
    GDExtensionClassCreationInfo6 class_info = {0};
    class_info.is_virtual = false;
    class_info.is_abstract = false;
    class_info.is_exposed = true;
    class_info.is_runtime = true;
    class_info.icon_path = nullptr;
    class_info.class_userdata = nullptr;
    
    /* Instance binding callbacks */
    class_info.set_func = mansion_adapter_set;
    class_info.get_func = mansion_adapter_get;
    class_info.get_property_list_func = mansion_adapter_get_property_list;
    class_info.free_property_list_func = mansion_adapter_free_property_list;
    class_info.property_can_revert_func = mansion_adapter_property_can_revert;
    class_info.property_get_revert_func = mansion_adapter_property_get_revert;
    class_info.validate_property_func = nullptr;
    class_info.notification_func = mansion_adapter_notification;
    class_info.to_string_func = mansion_adapter_to_string;
    class_info.reference_func = mansion_adapter_reference;
    class_info.unreference_func = mansion_adapter_unreference;
    
    /* Instance creation */
    class_info.create_instance_func = mansion_adapter_create_instance;
    class_info.free_instance_func = mansion_adapter_free_instance;
    class_info.recreate_instance_func = mansion_adapter_recreate_instance;
    
    /* Virtual method handling */
    class_info.get_virtual_func = nullptr;
    class_info.get_virtual_call_data_func = nullptr;
    class_info.call_virtual_with_data_func = nullptr;
    
    /* Register the class */
    if (classdb_register_extension_class6) {
        classdb_register_extension_class6(p_library, class_name_sn, parent_name_sn, &class_info);
    }
    
    /* Register methods */
    GDExtensionUninitializedStringNamePtr method_name_sn_ptr;
    
    /* initialize method */
    GDExtensionClassMethodInfo method_info;
    memset(&method_info, 0, sizeof(method_info));
    method_info.method_flags = GDEXTENSION_METHOD_FLAG_NORMAL;
    method_info.has_return_value = true;
    method_info.return_value_info = nullptr;
    method_info.return_value_metadata = GDEXTENSION_METHOD_ARGUMENT_METADATA_NONE;
    method_info.argument_count = 2;
    method_info.arguments_info = nullptr;
    method_info.arguments_metadata = nullptr;
    method_info.default_argument_count = 0;
    method_info.default_arguments = nullptr;
    
    string_name_new_with_latin1_chars(method_name_sn_ptr, "initialize", false);
    GDExtensionStringNamePtr method_name_sn = (GDExtensionStringNamePtr)method_name_sn_ptr;
    method_info.name = method_name_sn;
    method_info.method_userdata = nullptr;
    method_info.call_func = mansion_adapter_initialize_call;
    method_info.ptrcall_func = nullptr;
    
    if (classdb_register_extension_class_method) {
        classdb_register_extension_class_method(p_library, class_name_sn, &method_info);
    }
    
    /* shutdown method */
    memset(&method_info, 0, sizeof(method_info));
    method_info.method_flags = GDEXTENSION_METHOD_FLAG_NORMAL;
    method_info.has_return_value = false;
    method_info.argument_count = 0;
    
    string_name_new_with_latin1_chars(method_name_sn_ptr, "shutdown", false);
    method_info.name = method_name_sn;
    method_info.method_userdata = nullptr;
    method_info.call_func = mansion_adapter_shutdown_call;
    method_info.ptrcall_func = nullptr;
    
    if (classdb_register_extension_class_method) {
        classdb_register_extension_class_method(p_library, class_name_sn, &method_info);
    }
    
    /* destroy method */
    memset(&method_info, 0, sizeof(method_info));
    method_info.method_flags = GDEXTENSION_METHOD_FLAG_NORMAL;
    method_info.has_return_value = false;
    method_info.argument_count = 0;
    
    string_name_new_with_latin1_chars(method_name_sn_ptr, "destroy", false);
    method_info.name = method_name_sn;
    method_info.method_userdata = nullptr;
    method_info.call_func = mansion_adapter_destroy_call;
    method_info.ptrcall_func = nullptr;
    
    if (classdb_register_extension_class_method) {
        classdb_register_extension_class_method(p_library, class_name_sn, &method_info);
    }
    
    /* pump method */
    memset(&method_info, 0, sizeof(method_info));
    method_info.method_flags = GDEXTENSION_METHOD_FLAG_NORMAL;
    method_info.has_return_value = true;
    method_info.return_value_info = nullptr;
    method_info.return_value_metadata = GDEXTENSION_METHOD_ARGUMENT_METADATA_NONE;
    method_info.argument_count = 0;
    
    string_name_new_with_latin1_chars(method_name_sn_ptr, "pump", false);
    method_info.name = method_name_sn;
    method_info.method_userdata = nullptr;
    method_info.call_func = mansion_adapter_pump_call;
    method_info.ptrcall_func = nullptr;
    
    if (classdb_register_extension_class_method) {
        classdb_register_extension_class_method(p_library, class_name_sn, &method_info);
    }
    
    /* flush method */
    memset(&method_info, 0, sizeof(method_info));
    method_info.method_flags = GDEXTENSION_METHOD_FLAG_NORMAL;
    method_info.has_return_value = true;
    method_info.return_value_info = nullptr;
    method_info.return_value_metadata = GDEXTENSION_METHOD_ARGUMENT_METADATA_NONE;
    method_info.argument_count = 0;
    
    string_name_new_with_latin1_chars(method_name_sn_ptr, "flush", false);
    method_info.name = method_name_sn;
    method_info.method_userdata = nullptr;
    method_info.call_func = mansion_adapter_flush_call;
    method_info.ptrcall_func = nullptr;
    
    if (classdb_register_extension_class_method) {
        classdb_register_extension_class_method(p_library, class_name_sn, &method_info);
    }
    
    /* get_focused_serial method */
    memset(&method_info, 0, sizeof(method_info));
    method_info.method_flags = GDEXTENSION_METHOD_FLAG_NORMAL;
    method_info.has_return_value = true;
    method_info.return_value_info = nullptr;
    method_info.return_value_metadata = GDEXTENSION_METHOD_ARGUMENT_METADATA_NONE;
    method_info.argument_count = 0;
    
    string_name_new_with_latin1_chars(method_name_sn_ptr, "get_focused_serial", false);
    method_info.name = method_name_sn;
    method_info.method_userdata = nullptr;
    method_info.call_func = mansion_adapter_get_focused_serial_call;
    method_info.ptrcall_func = nullptr;
    
    if (classdb_register_extension_class_method) {
        classdb_register_extension_class_method(p_library, class_name_sn, &method_info);
    }
}

/* Module initialization - this is the actual GDExtension entry point */
static GDExtensionBool mansion_extension_init(
    GDExtensionInterfaceGetProcAddress p_get_proc_address,
    GDExtensionClassLibraryPtr p_library,
    GDExtensionInitialization *r_initialization) {
    
    fprintf(stderr, "[mansion] Initialization started\n");
    
    /* Initialize interface function pointers */
    init_interface_functions(p_get_proc_address);
    
    /* Register our class */
    mansion_register_class(p_library);
    
    /* Set initialization level */
    r_initialization->minimum_initialization_level = GDEXTENSION_INITIALIZATION_CORE;
    r_initialization->userdata = nullptr;
    r_initialization->initialize = nullptr;
    r_initialization->deinitialize = nullptr;
    
    fprintf(stderr, "[mansion] Initialization complete\n");
    return true;
}

/* GDExtension entry point - must be exported as C symbol */
extern "C" {

GDExtensionBool GDExtensionInit(
    GDExtensionInterfaceGetProcAddress p_get_proc_address,
    GDExtensionClassLibraryPtr p_library,
    GDExtensionInitialization *r_initialization) {
    
    return mansion_extension_init(p_get_proc_address, p_library, r_initialization);
}

} /* extern "C" */
