/*
 * ElsewhereExtension Minimal — GDExtension entry point for testing
 *
 * This file provides a minimal GDExtension that doesn't use any external
 * libraries to test if the hang is caused by library loading issues.
 */

#include <gdextension_interface.h>
#include <cstdio>
#include <cstring>

/* GDExtension interface function pointers */
static GDExtensionInterfaceGetProcAddress gdext_get_proc_address = nullptr;

/* Cached function pointers */
static GDExtensionInterfaceClassdbRegisterExtensionClass6 classdb_register_extension_class6 = nullptr;
static GDExtensionInterfaceClassdbRegisterExtensionClassMethod classdb_register_extension_class_method = nullptr;
static GDExtensionInterfaceStringNameNewWithLatin1Chars string_name_new_with_latin1_chars = nullptr;
static GDExtensionInterfaceStringNewWithLatin1Chars string_new_with_latin1_chars = nullptr;
static GDExtensionInterfaceVariantDestroy variant_destroy = nullptr;
static GDExtensionInterfaceVariantNewNil variant_new_nil = nullptr;

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
    variant_destroy = (GDExtensionInterfaceVariantDestroy)
        get_interface_function("variant_destroy");
    variant_new_nil = (GDExtensionInterfaceVariantNewNil)
        get_interface_function("variant_new_nil");
}

/* Minimal class that does nothing */
typedef struct {
    int dummy;
} MinimalInstance;

static GDExtensionClassInstancePtr minimal_create_instance(
    void *p_class_userdata,
    GDExtensionBool p_notify_postinitialize) {
    (void)p_class_userdata;
    (void)p_notify_postinitialize;
    return nullptr;
}

static void minimal_free_instance(void *p_class_userdata, GDExtensionClassInstancePtr p_instance) {
    (void)p_class_userdata;
    (void)p_instance;
}

static GDExtensionClassInstancePtr minimal_recreate_instance(
    void *p_class_userdata,
    GDExtensionObjectPtr p_object) {
    (void)p_class_userdata;
    (void)p_object;
    return nullptr;
}

static void minimal_reference(GDExtensionClassInstancePtr p_instance) {
    (void)p_instance;
}

static void minimal_unreference(GDExtensionClassInstancePtr p_instance) {
    (void)p_instance;
}

static GDExtensionBool minimal_set(
    GDExtensionClassInstancePtr p_instance,
    GDExtensionConstStringNamePtr p_name,
    GDExtensionConstVariantPtr p_value) {
    (void)p_instance;
    (void)p_name;
    (void)p_value;
    return false;
}

static GDExtensionBool minimal_get(
    GDExtensionClassInstancePtr p_instance,
    GDExtensionConstStringNamePtr p_name,
    GDExtensionVariantPtr r_ret) {
    (void)p_instance;
    (void)p_name;
    variant_new_nil(r_ret);
    return false;
}

static const GDExtensionPropertyInfo *minimal_get_property_list(
    GDExtensionClassInstancePtr p_instance,
    uint32_t *r_count) {
    (void)p_instance;
    *r_count = 0;
    return nullptr;
}

static void minimal_free_property_list(
    GDExtensionClassInstancePtr p_instance,
    const GDExtensionPropertyInfo *p_list,
    uint32_t p_count) {
    (void)p_instance;
    (void)p_list;
    (void)p_count;
}

static GDExtensionBool minimal_property_can_revert(
    GDExtensionClassInstancePtr p_instance,
    GDExtensionConstStringNamePtr p_name) {
    (void)p_instance;
    (void)p_name;
    return false;
}

static GDExtensionBool minimal_property_get_revert(
    GDExtensionClassInstancePtr p_instance,
    GDExtensionConstStringNamePtr p_name,
    GDExtensionVariantPtr r_ret) {
    (void)p_instance;
    (void)p_name;
    variant_new_nil(r_ret);
    return false;
}

static void minimal_notification(
    GDExtensionClassInstancePtr p_instance,
    int32_t p_what,
    GDExtensionBool p_reversed) {
    (void)p_instance;
    (void)p_what;
    (void)p_reversed;
}

static void minimal_to_string(
    GDExtensionClassInstancePtr p_instance,
    GDExtensionBool *r_is_valid,
    GDExtensionStringPtr p_out) {
    (void)p_instance;
    if (r_is_valid) {
        *r_is_valid = true;
    }
    if (p_out) {
        string_new_with_latin1_chars(p_out, "MinimalAdapter");
    }
}

static void minimal_method_call(
    void *method_userdata,
    GDExtensionClassInstancePtr p_instance,
    const GDExtensionConstVariantPtr *p_args,
    GDExtensionInt p_argument_count,
    GDExtensionVariantPtr r_return,
    GDExtensionCallError *r_error) {
    (void)method_userdata;
    (void)p_instance;
    (void)p_args;
    (void)r_return;
    if (r_error) {
        r_error->error = GDEXTENSION_CALL_OK;
    }
    if (p_argument_count != 0) {
        if (r_error) {
            r_error->error = GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS;
        }
        return;
    }
}

static void minimal_register_class(GDExtensionClassLibraryPtr p_library) {
    static const char class_name[] = "MinimalAdapter";
    static const char parent_class[] = "RefCounted";
    
    GDExtensionUninitializedStringNamePtr class_name_sn_storage = nullptr;
    GDExtensionUninitializedStringNamePtr parent_name_sn_storage = nullptr;
    
    string_name_new_with_latin1_chars(&class_name_sn_storage, class_name, false);
    string_name_new_with_latin1_chars(&parent_name_sn_storage, parent_class, false);
    
    GDExtensionStringNamePtr class_name_sn = class_name_sn_storage;
    GDExtensionStringNamePtr parent_name_sn = parent_name_sn_storage;
    
    GDExtensionClassCreationInfo6 class_info = {0};
    class_info.is_virtual = false;
    class_info.is_abstract = false;
    class_info.is_exposed = true;
    class_info.is_runtime = true;
    class_info.icon_path = nullptr;
    class_info.class_userdata = nullptr;
    class_info.set_func = minimal_set;
    class_info.get_func = minimal_get;
    class_info.get_property_list_func = minimal_get_property_list;
    class_info.free_property_list_func = minimal_free_property_list;
    class_info.property_can_revert_func = minimal_property_can_revert;
    class_info.property_get_revert_func = minimal_property_get_revert;
    class_info.validate_property_func = nullptr;
    class_info.notification_func = minimal_notification;
    class_info.to_string_func = minimal_to_string;
    class_info.reference_func = minimal_reference;
    class_info.unreference_func = minimal_unreference;
    class_info.create_instance_func = minimal_create_instance;
    class_info.free_instance_func = minimal_free_instance;
    class_info.recreate_instance_func = minimal_recreate_instance;
    class_info.get_virtual_func = nullptr;
    class_info.get_virtual_call_data_func = nullptr;
    class_info.call_virtual_with_data_func = nullptr;
    
    if (classdb_register_extension_class6) {
        classdb_register_extension_class6(p_library, class_name_sn, parent_name_sn, &class_info);
    }
    
    GDExtensionUninitializedStringNamePtr method_name_sn_storage = nullptr;
    
    GDExtensionClassMethodInfo method_info;
    memset(&method_info, 0, sizeof(GDExtensionClassMethodInfo));
    method_info.method_flags = GDEXTENSION_METHOD_FLAG_NORMAL;
    method_info.has_return_value = false;
    method_info.argument_count = 0;
    
    string_name_new_with_latin1_chars(&method_name_sn_storage, "test", false);
    method_info.name = method_name_sn_storage;
    method_info.method_userdata = nullptr;
    method_info.call_func = minimal_method_call;
    method_info.ptrcall_func = nullptr;
    
    if (classdb_register_extension_class_method) {
        classdb_register_extension_class_method(p_library, class_name_sn, &method_info);
    }
}

static GDExtensionBool minimal_extension_init(
    GDExtensionInterfaceGetProcAddress p_get_proc_address,
    GDExtensionClassLibraryPtr p_library,
    GDExtensionInitialization *r_initialization) {
    
    FILE *log = fopen("/tmp/gdextension_init.log", "a");
    if (log) {
        fprintf(log, "[minimal] Initialization started\n");
        fflush(log);
    }
    
    init_interface_functions(p_get_proc_address);
    
    if (log) fprintf(log, "[minimal] Registering class\n");
    minimal_register_class(p_library);
    
    if (log) fprintf(log, "[minimal] Setting initialization level\n");
    r_initialization->minimum_initialization_level = GDEXTENSION_INITIALIZATION_SERVERS;
    r_initialization->userdata = nullptr;
    r_initialization->initialize = nullptr;
    r_initialization->deinitialize = nullptr;
    
    if (log) {
        fprintf(log, "[minimal] Initialization complete\n");
        fflush(log);
        fclose(log);
    }
    return true;
}

extern "C" {

GDExtensionBool __attribute__((visibility("default"))) GDExtensionInit(
    GDExtensionInterfaceGetProcAddress p_get_proc_address,
    GDExtensionClassLibraryPtr p_library,
    GDExtensionInitialization *r_initialization) {
    
    return minimal_extension_init(p_get_proc_address, p_library, r_initialization);
}

} /* extern "C" */
