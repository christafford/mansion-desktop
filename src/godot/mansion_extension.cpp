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
static GDExtensionInterfaceStringToUtf8Chars string_to_utf8_chars = nullptr;
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
    string_to_utf8_chars = (GDExtensionInterfaceStringToUtf8Chars)
        get_interface_function("string_to_utf8_chars");
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
    int ref_count;
} MansionAdapterInstance;

/* GDExtensionClassCreateInstance3 - creates a new instance */
static GDExtensionClassInstancePtr mansion_adapter_create_instance(
    void *p_class_userdata,
    GDExtensionBool p_notify_postinitialize) {
    (void)p_class_userdata;
    (void)p_notify_postinitialize;
    
    /* Allocate and initialize the instance */
    MansionAdapterInstance *instance = (MansionAdapterInstance *)mem_alloc(sizeof(MansionAdapterInstance));
    if (!instance) {
        return nullptr;
    }
    
    instance->adapter = nullptr;
    instance->ref_count = 1;
    
    return (GDExtensionClassInstancePtr)instance;
}

/* GDExtensionClassFreeInstance - frees an instance */
static void mansion_adapter_free_instance(void *p_class_userdata, GDExtensionClassInstancePtr p_instance) {
    (void)p_class_userdata;
    
    if (!p_instance) {
        return;
    }
    
    MansionAdapterInstance *instance = (MansionAdapterInstance *)p_instance;
    
    /* Destroy the adapter if it exists */
    if (instance->adapter) {
        mansion_gdextension_adapter_destroy(instance->adapter);
        instance->adapter = nullptr;
    }
    
    /* Free the instance */
    mem_free(instance);
}

/* GDExtensionClassRecreateInstance - recreates an instance */
static GDExtensionClassInstancePtr mansion_adapter_recreate_instance(
    void *p_class_userdata,
    GDExtensionObjectPtr p_object) {
    (void)p_class_userdata;
    (void)p_object;
    
    /* Recreate with default state */
    return mansion_adapter_create_instance(p_class_userdata, false);
}

/* GDExtensionClassReference - increases reference count */
static void mansion_adapter_reference(GDExtensionClassInstancePtr p_instance) {
    if (!p_instance) {
        return;
    }
    MansionAdapterInstance *instance = (MansionAdapterInstance *)p_instance;
    instance->ref_count++;
}

/* GDExtensionClassUnreference - decreases reference count */
static void mansion_adapter_unreference(GDExtensionClassInstancePtr p_instance) {
    if (!p_instance) {
        return;
    }
    MansionAdapterInstance *instance = (MansionAdapterInstance *)p_instance;
    instance->ref_count--;
}

/* GDExtensionClassSet - sets a property value, returns GDExtensionBool */
static GDExtensionBool mansion_adapter_set(
    GDExtensionClassInstancePtr p_instance,
    GDExtensionConstStringNamePtr p_name,
    GDExtensionConstVariantPtr p_value) {
    (void)p_instance;
    (void)p_name;
    (void)p_value;
    return false; /* No properties to set */
}

/* GDExtensionClassGet - gets a property value, returns GDExtensionBool */
static GDExtensionBool mansion_adapter_get(
    GDExtensionClassInstancePtr p_instance,
    GDExtensionConstStringNamePtr p_name,
    GDExtensionVariantPtr r_ret) {
    (void)p_instance;
    (void)p_name;
    variant_new_nil(r_ret);
    return false; /* No properties to get */
}

/* GDExtensionClassGetPropertyList - returns property list, or nullptr if none */
static const GDExtensionPropertyInfo *mansion_adapter_get_property_list(
    GDExtensionClassInstancePtr p_instance,
    uint32_t *r_count) {
    (void)p_instance;
    *r_count = 0;
    return nullptr;
}

/* GDExtensionClassFreePropertyList2 - frees property list */
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
    return false; /* No properties can revert */
}

/* GDExtensionClassPropertyGetRevert - returns GDExtensionBool */
static GDExtensionBool mansion_adapter_property_get_revert(
    GDExtensionClassInstancePtr p_instance,
    GDExtensionConstStringNamePtr p_name,
    GDExtensionVariantPtr r_ret) {
    (void)p_instance;
    (void)p_name;
    variant_new_nil(r_ret);
    return false; /* No properties to revert */
}

/* GDExtensionClassNotification2 - receives notifications */
static void mansion_adapter_notification(
    GDExtensionClassInstancePtr p_instance,
    int32_t p_what,
    GDExtensionBool p_reversed) {
    (void)p_instance;
    (void)p_what;
    (void)p_reversed;
}

/* GDExtensionClassToString - converts to string */
static void mansion_adapter_to_string(
    GDExtensionClassInstancePtr p_instance,
    GDExtensionBool *r_is_valid,
    GDExtensionStringPtr p_out) {
    (void)p_instance;
    if (r_is_valid) {
        *r_is_valid = true;
    }
    if (p_out) {
        string_new_with_latin1_chars(p_out, "MansionAdapter");
    }
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
    
    if (!instance) {
        if (r_error) {
            r_error->error = GDEXTENSION_CALL_ERROR_INSTANCE_IS_NULL;
        }
        return;
    }
    
    /* Get the socket name and debug flag from arguments */
    GDExtensionVariantType arg_type0 = variant_get_type(p_args[0]);
    GDExtensionVariantType arg_type1 = variant_get_type(p_args[1]);
    
    if (arg_type0 != GDEXTENSION_VARIANT_TYPE_STRING || arg_type1 != GDEXTENSION_VARIANT_TYPE_BOOL) {
        if (r_error) {
            r_error->error = GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT;
        }
        return;
    }
    
    /* Extract socket name from Variant String */
    /* In GDExtension, String is stored as a pointer in Variant */
    GDExtensionStringPtr socket_str_ptr = *(GDExtensionStringPtr *)p_args[0];
    
    /* Get the UTF-8 characters from the string */
    /* First get the required length */
    GDExtensionInt str_len = string_to_utf8_chars(socket_str_ptr, nullptr, 0);
    
    /* Allocate buffer for the string (plus null terminator) */
    char *socket_name_cstr = (char *)mem_alloc(str_len + 1);
    if (!socket_name_cstr) {
        if (r_error) {
            r_error->error = GDEXTENSION_CALL_ERROR_INVALID_METHOD;
        }
        return;
    }
    
    /* Convert to C string */
    string_to_utf8_chars(socket_str_ptr, socket_name_cstr, str_len);
    socket_name_cstr[str_len] = '\0';
    
    /* Create the adapter */
    MansionGDExtensionConfig config;
    memset(&config, 0, sizeof(config));
    
    config.socket_name = socket_name_cstr;
    config.debug_logging = *(GDExtensionBool *)p_args[1];
    
    instance->adapter = mansion_gdextension_adapter_create(&config);
    
    /* Free the temporary buffer */
    mem_free(socket_name_cstr);
    
    if (!instance->adapter) {
        if (r_error) {
            r_error->error = GDEXTENSION_CALL_ERROR_INVALID_METHOD;
        }
        GDExtensionBool result = false;
        memcpy(r_return, &result, sizeof(GDExtensionBool));
        return;
    }
    
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
    GDExtensionUninitializedStringNamePtr class_name_sn_storage = nullptr;
    GDExtensionUninitializedStringNamePtr parent_name_sn_storage = nullptr;
    
    /* Always initialize string names - the function is always available in Godot 4.5+ */
    string_name_new_with_latin1_chars(&class_name_sn_storage, class_name, false);
    string_name_new_with_latin1_chars(&parent_name_sn_storage, parent_class, false);
    
    GDExtensionStringNamePtr class_name_sn = class_name_sn_storage;
    GDExtensionStringNamePtr parent_name_sn = parent_name_sn_storage;
    
    /* Setup GDExtensionClassCreationInfo6 */
    GDExtensionClassCreationInfo6 class_info = {0};
    class_info.is_virtual = false;
    class_info.is_abstract = false;
    class_info.is_exposed = true;
    class_info.is_runtime = true;
    class_info.icon_path = nullptr;
    class_info.class_userdata = nullptr;
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
    class_info.create_instance_func = mansion_adapter_create_instance;
    class_info.free_instance_func = mansion_adapter_free_instance;
    class_info.recreate_instance_func = mansion_adapter_recreate_instance;
    class_info.get_virtual_func = nullptr;
    class_info.get_virtual_call_data_func = nullptr;
    class_info.call_virtual_with_data_func = nullptr;
    
    /* Register the class */
    if (classdb_register_extension_class6) {
        classdb_register_extension_class6(p_library, class_name_sn, parent_name_sn, &class_info);
    }
    
    /* Register methods */
    GDExtensionUninitializedStringNamePtr method_name_sn_storage = nullptr;
    
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
    
    string_name_new_with_latin1_chars(&method_name_sn_storage, "initialize", false);
    GDExtensionStringNamePtr method_name_sn = method_name_sn_storage;
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
    
    string_name_new_with_latin1_chars(&method_name_sn_storage, "shutdown", false);
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
    
    string_name_new_with_latin1_chars(&method_name_sn_storage, "destroy", false);
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
    
    string_name_new_with_latin1_chars(&method_name_sn_storage, "pump", false);
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
    
    string_name_new_with_latin1_chars(&method_name_sn_storage, "flush", false);
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
    
    string_name_new_with_latin1_chars(&method_name_sn_storage, "get_focused_serial", false);
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
    
    FILE *log = fopen("/tmp/gdextension_init.log", "a");
    if (log) {
        fprintf(log, "[mansion] Initialization started\n");
        fflush(log);
    }
    
    /* Initialize interface function pointers */
    if (log) fprintf(log, "[mansion] Initializing interface functions\n");
    init_interface_functions(p_get_proc_address);
    
    if (log) fprintf(log, "[mansion] Registering class\n");
    /* Register our class */
    mansion_register_class(p_library);
    
    if (log) fprintf(log, "[mansion] Setting initialization level\n");
    /* Set initialization level */
    r_initialization->minimum_initialization_level = GDEXTENSION_INITIALIZATION_CORE;
    r_initialization->userdata = nullptr;
    r_initialization->initialize = nullptr;
    r_initialization->deinitialize = nullptr;
    
    if (log) {
        fprintf(log, "[mansion] Initialization complete\n");
        fflush(log);
        fclose(log);
    }
    return true;
}

/* GDExtension entry point - must be exported as C symbol with default visibility */
extern "C" {

/* Mark GDExtensionInit with default visibility even when -fvisibility=hidden is used */
GDExtensionBool __attribute__((visibility("default"))) GDExtensionInit(
    GDExtensionInterfaceGetProcAddress p_get_proc_address,
    GDExtensionClassLibraryPtr p_library,
    GDExtensionInitialization *r_initialization) {
    
    return mansion_extension_init(p_get_proc_address, p_library, r_initialization);
}

/* Mark GDExtensionExtensionDestroy with default visibility even when -fvisibility=hidden is used */
void __attribute__((visibility("default"))) GDExtensionExtensionDestroy(GDExtensionClassLibraryPtr p_library) {
    /* No cleanup needed - the extension doesn't allocate global resources */
}

} /* extern "C" */
