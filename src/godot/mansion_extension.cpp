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
    /* Open log file for each message to ensure we see output even on crash */
    FILE *log = fopen("/tmp/gdextension_init.log", "a");
    if (log) {
        fprintf(log, "[mansion] get_interface_function: %s (interface: %p)\n", name, (void *)gdext_get_proc_address);
        fflush(log);
        fclose(log);
    }
    
    /* Check if the interface function pointer is valid before calling it */
    if (!gdext_get_proc_address) {
        log = fopen("/tmp/gdextension_init.log", "a");
        if (log) {
            fprintf(log, "[mansion] ERROR: gdext_get_proc_address is null\n");
            fflush(log);
            fclose(log);
        }
        return nullptr;
    }
    
    /* Call the interface function - if this crashes, the pointer is invalid */
    void *result = (void *)gdext_get_proc_address(name);
    
    log = fopen("/tmp/gdextension_init.log", "a");
    if (log) {
        fprintf(log, "[mansion] get_interface_function: %s -> %p\n", name, result);
        fflush(log);
        fclose(log);
    }
    
    return result;
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
    
    /* Get the debug flag */
    GDExtensionBool debug_flag = *(GDExtensionBool *)p_args[1];
    
    /* Create config for adapter creation */
    MansionGDExtensionConfig config;
    memset(&config, 0, sizeof(config));
    config.socket_name = socket_name_cstr;
    config.debug_logging = debug_flag == 1;
    
    /* Create the adapter */
    instance->adapter = mansion_gdextension_adapter_create(&config);
    
    /* Free the socket name string */
    mem_free(socket_name_cstr);
    
    /* Set the result */
    GDExtensionBool result_val = instance->adapter ? 1 : 0;
    memcpy(r_return, &result_val, sizeof(GDExtensionBool));
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
    
    /* Validate argument count */
    if (p_argument_count != 0) {
        if (r_error) {
            r_error->error = GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS;
            r_error->argument = 0;
            r_error->expected = 0;
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
    
    /* Shutdown the adapter */
    if (instance->adapter) {
        mansion_gdextension_adapter_destroy(instance->adapter);
        instance->adapter = nullptr;
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
    
    /* Validate argument count */
    if (p_argument_count != 0) {
        if (r_error) {
            r_error->error = GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS;
            r_error->argument = 0;
            r_error->expected = 0;
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
    
    /* Destroy the adapter */
    if (instance->adapter) {
        mansion_gdextension_adapter_destroy(instance->adapter);
        instance->adapter = nullptr;
    }
}

/* Method: pump() -> void */
static void mansion_adapter_pump_call(
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
    
    /* Validate argument count */
    if (p_argument_count != 0) {
        if (r_error) {
            r_error->error = GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS;
            r_error->argument = 0;
            r_error->expected = 0;
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
    
    /* Pump events */
    if (instance->adapter) {
        mansion_gdextension_adapter_pump(instance->adapter);
    }
}

/* Method: flush() -> void */
static void mansion_adapter_flush_call(
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
    
    /* Validate argument count */
    if (p_argument_count != 0) {
        if (r_error) {
            r_error->error = GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS;
            r_error->argument = 0;
            r_error->expected = 0;
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
    
    /* Flush events */
    if (instance->adapter) {
        mansion_gdextension_adapter_flush(instance->adapter);
    }
}

/* Method: get_focused_serial() -> int */
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
    
    /* Validate argument count */
    if (p_argument_count != 0) {
        if (r_error) {
            r_error->error = GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS;
            r_error->argument = 0;
            r_error->expected = 0;
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
    
    if (!instance || !instance->adapter) {
        GDExtensionInt result = 0;
        memcpy(r_return, &result, sizeof(GDExtensionInt));
        return;
    }
    
    uint32_t result = mansion_gdextension_adapter_get_focused_serial(instance->adapter);
    GDExtensionInt result_val = result;
    memcpy(r_return, &result_val, sizeof(GDExtensionInt));
}

/* Method: set_compositor(display_ptr: uint64, compositor_ptr: uint64) -> bool */
static void mansion_adapter_set_compositor_call(
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
    
    /* Get the display and compositor pointers from arguments */
    GDExtensionVariantType arg_type0 = variant_get_type(p_args[0]);
    GDExtensionVariantType arg_type1 = variant_get_type(p_args[1]);
    
    if (arg_type0 != GDEXTENSION_VARIANT_TYPE_INT || arg_type1 != GDEXTENSION_VARIANT_TYPE_INT) {
        if (r_error) {
            r_error->error = GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT;
        }
        return;
    }
    
    /* Extract uint64_t values from Variant */
    GDExtensionInt display_ptr = 0, compositor_ptr = 0;
    memcpy(&display_ptr, p_args[0], sizeof(GDExtensionInt));
    memcpy(&compositor_ptr, p_args[1], sizeof(GDExtensionInt));
    
    /* Set the display and compositor in the adapter */
    if (instance->adapter) {
        /* Cast uint64_t back to pointers */
        instance->adapter->display = (struct wl_display*)(uintptr_t)display_ptr;
        instance->adapter->compositor = (struct MansionCompositor*)(uintptr_t)compositor_ptr;
        
        /* Re-initialize the event loop with the new display */
        instance->adapter->event_loop = wl_display_get_event_loop(instance->adapter->display);
        
        GDExtensionBool result_val = 1;
        memcpy(r_return, &result_val, sizeof(GDExtensionBool));
    } else {
        GDExtensionBool result_val = 0;
        memcpy(r_return, &result_val, sizeof(GDExtensionBool));
    }
}

/* ─── Shm frame snapshot methods (P21-T12) ─── */

/* Method: create_snapshot(client_serial: int) -> uint64 (snapshot pointer) */
static void mansion_adapter_create_snapshot_call(
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
    
    if (p_argument_count != 1) {
        if (r_error) {
            r_error->error = GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS;
            r_error->argument = 1;
            r_error->expected = 1;
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
    
    GDExtensionVariantType arg_type0 = variant_get_type(p_args[0]);
    if (arg_type0 != GDEXTENSION_VARIANT_TYPE_INT) {
        if (r_error) {
            r_error->error = GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT;
        }
        return;
    }
    
    GDExtensionInt client_serial_int = 0;
    memcpy(&client_serial_int, p_args[0], sizeof(GDExtensionInt));
    
    uint32_t client_serial = (uint32_t)client_serial_int;
    
    MansionShmSnapshot *snapshot = mansion_gdextension_adapter_create_snapshot(
        instance->adapter, client_serial);
    
    /* Store the snapshot pointer as a uint64 in the return value */
    GDExtensionInt snapshot_ptr_val = (GDExtensionInt)(uintptr_t)snapshot;
    memcpy(r_return, &snapshot_ptr_val, sizeof(GDExtensionInt));
}

/* Method: snapshot_destroy(snapshot_ptr: uint64) -> void */
static void mansion_adapter_snapshot_destroy_call(
    void *method_userdata,
    GDExtensionClassInstancePtr p_instance,
    const GDExtensionConstVariantPtr *p_args,
    GDExtensionInt p_argument_count,
    GDExtensionVariantPtr r_return,
    GDExtensionCallError *r_error) {
    
    (void)method_userdata;
    (void)p_instance;
    
    if (r_error) {
        r_error->error = GDEXTENSION_CALL_OK;
    }
    
    if (p_argument_count != 1) {
        if (r_error) {
            r_error->error = GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS;
            r_error->argument = 1;
            r_error->expected = 1;
        }
        return;
    }
    
    GDExtensionVariantType arg_type0 = variant_get_type(p_args[0]);
    if (arg_type0 != GDEXTENSION_VARIANT_TYPE_INT) {
        if (r_error) {
            r_error->error = GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT;
        }
        return;
    }
    
    GDExtensionInt snapshot_ptr_int = 0;
    memcpy(&snapshot_ptr_int, p_args[0], sizeof(GDExtensionInt));
    
    MansionShmSnapshot *snapshot = (MansionShmSnapshot *)(uintptr_t)snapshot_ptr_int;
    if (snapshot) {
        mansion_gdextension_adapter_destroy_snapshot(snapshot);
    }
}

/* Method: snapshot_get_width(snapshot_ptr: uint64) -> int */
static void mansion_adapter_snapshot_get_width_call(
    void *method_userdata,
    GDExtensionClassInstancePtr p_instance,
    const GDExtensionConstVariantPtr *p_args,
    GDExtensionInt p_argument_count,
    GDExtensionVariantPtr r_return,
    GDExtensionCallError *r_error) {
    
    (void)method_userdata;
    (void)p_instance;
    
    if (r_error) {
        r_error->error = GDEXTENSION_CALL_OK;
    }
    
    if (p_argument_count != 1) {
        if (r_error) {
            r_error->error = GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS;
            r_error->argument = 1;
            r_error->expected = 1;
        }
        return;
    }
    
    GDExtensionVariantType arg_type0 = variant_get_type(p_args[0]);
    if (arg_type0 != GDEXTENSION_VARIANT_TYPE_INT) {
        if (r_error) {
            r_error->error = GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT;
        }
        return;
    }
    
    GDExtensionInt snapshot_ptr_int = 0;
    memcpy(&snapshot_ptr_int, p_args[0], sizeof(GDExtensionInt));
    
    MansionShmSnapshot *snapshot = (MansionShmSnapshot *)(uintptr_t)snapshot_ptr_int;
    int32_t width = snapshot ? mansion_gdextension_snapshot_get_width(snapshot) : 0;
    GDExtensionInt width_val = width;
    memcpy(r_return, &width_val, sizeof(GDExtensionInt));
}

/* Method: snapshot_get_height(snapshot_ptr: uint64) -> int */
static void mansion_adapter_snapshot_get_height_call(
    void *method_userdata,
    GDExtensionClassInstancePtr p_instance,
    const GDExtensionConstVariantPtr *p_args,
    GDExtensionInt p_argument_count,
    GDExtensionVariantPtr r_return,
    GDExtensionCallError *r_error) {
    
    (void)method_userdata;
    (void)p_instance;
    
    if (r_error) {
        r_error->error = GDEXTENSION_CALL_OK;
    }
    
    if (p_argument_count != 1) {
        if (r_error) {
            r_error->error = GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS;
            r_error->argument = 1;
            r_error->expected = 1;
        }
        return;
    }
    
    GDExtensionVariantType arg_type0 = variant_get_type(p_args[0]);
    if (arg_type0 != GDEXTENSION_VARIANT_TYPE_INT) {
        if (r_error) {
            r_error->error = GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT;
        }
        return;
    }
    
    GDExtensionInt snapshot_ptr_int = 0;
    memcpy(&snapshot_ptr_int, p_args[0], sizeof(GDExtensionInt));
    
    MansionShmSnapshot *snapshot = (MansionShmSnapshot *)(uintptr_t)snapshot_ptr_int;
    int32_t height = snapshot ? mansion_gdextension_snapshot_get_height(snapshot) : 0;
    GDExtensionInt height_val = height;
    memcpy(r_return, &height_val, sizeof(GDExtensionInt));
}

/* Method: snapshot_get_stride(snapshot_ptr: uint64) -> int */
static void mansion_adapter_snapshot_get_stride_call(
    void *method_userdata,
    GDExtensionClassInstancePtr p_instance,
    const GDExtensionConstVariantPtr *p_args,
    GDExtensionInt p_argument_count,
    GDExtensionVariantPtr r_return,
    GDExtensionCallError *r_error) {
    
    (void)method_userdata;
    (void)p_instance;
    
    if (r_error) {
        r_error->error = GDEXTENSION_CALL_OK;
    }
    
    if (p_argument_count != 1) {
        if (r_error) {
            r_error->error = GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS;
            r_error->argument = 1;
            r_error->expected = 1;
        }
        return;
    }
    
    GDExtensionVariantType arg_type0 = variant_get_type(p_args[0]);
    if (arg_type0 != GDEXTENSION_VARIANT_TYPE_INT) {
        if (r_error) {
            r_error->error = GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT;
        }
        return;
    }
    
    GDExtensionInt snapshot_ptr_int = 0;
    memcpy(&snapshot_ptr_int, p_args[0], sizeof(GDExtensionInt));
    
    MansionShmSnapshot *snapshot = (MansionShmSnapshot *)(uintptr_t)snapshot_ptr_int;
    int32_t stride = snapshot ? mansion_gdextension_snapshot_get_stride(snapshot) : 0;
    GDExtensionInt stride_val = stride;
    memcpy(r_return, &stride_val, sizeof(GDExtensionInt));
}

/* Method: snapshot_get_format(snapshot_ptr: uint64) -> int */
static void mansion_adapter_snapshot_get_format_call(
    void *method_userdata,
    GDExtensionClassInstancePtr p_instance,
    const GDExtensionConstVariantPtr *p_args,
    GDExtensionInt p_argument_count,
    GDExtensionVariantPtr r_return,
    GDExtensionCallError *r_error) {
    
    (void)method_userdata;
    (void)p_instance;
    
    if (r_error) {
        r_error->error = GDEXTENSION_CALL_OK;
    }
    
    if (p_argument_count != 1) {
        if (r_error) {
            r_error->error = GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS;
            r_error->argument = 1;
            r_error->expected = 1;
        }
        return;
    }
    
    GDExtensionVariantType arg_type0 = variant_get_type(p_args[0]);
    if (arg_type0 != GDEXTENSION_VARIANT_TYPE_INT) {
        if (r_error) {
            r_error->error = GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT;
        }
        return;
    }
    
    GDExtensionInt snapshot_ptr_int = 0;
    memcpy(&snapshot_ptr_int, p_args[0], sizeof(GDExtensionInt));
    
    MansionShmSnapshot *snapshot = (MansionShmSnapshot *)(uintptr_t)snapshot_ptr_int;
    int32_t format = snapshot ? mansion_gdextension_snapshot_get_format(snapshot) : 0;
    GDExtensionInt format_val = format;
    memcpy(r_return, &format_val, sizeof(GDExtensionInt));
}

/* Method: snapshot_get_revision(snapshot_ptr: uint64) -> int */
static void mansion_adapter_snapshot_get_revision_call(
    void *method_userdata,
    GDExtensionClassInstancePtr p_instance,
    const GDExtensionConstVariantPtr *p_args,
    GDExtensionInt p_argument_count,
    GDExtensionVariantPtr r_return,
    GDExtensionCallError *r_error) {
    
    (void)method_userdata;
    (void)p_instance;
    
    if (r_error) {
        r_error->error = GDEXTENSION_CALL_OK;
    }
    
    if (p_argument_count != 1) {
        if (r_error) {
            r_error->error = GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS;
            r_error->argument = 1;
            r_error->expected = 1;
        }
        return;
    }
    
    GDExtensionVariantType arg_type0 = variant_get_type(p_args[0]);
    if (arg_type0 != GDEXTENSION_VARIANT_TYPE_INT) {
        if (r_error) {
            r_error->error = GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT;
        }
        return;
    }
    
    GDExtensionInt snapshot_ptr_int = 0;
    memcpy(&snapshot_ptr_int, p_args[0], sizeof(GDExtensionInt));
    
    MansionShmSnapshot *snapshot = (MansionShmSnapshot *)(uintptr_t)snapshot_ptr_int;
    uint32_t revision = snapshot ? mansion_gdextension_snapshot_get_revision(snapshot) : 0;
    GDExtensionInt revision_val = revision;
    memcpy(r_return, &revision_val, sizeof(GDExtensionInt));
}

/* Method: snapshot_get_pixels(snapshot_ptr: uint64) -> uint64 (pixels pointer) */
static void mansion_adapter_snapshot_get_pixels_call(
    void *method_userdata,
    GDExtensionClassInstancePtr p_instance,
    const GDExtensionConstVariantPtr *p_args,
    GDExtensionInt p_argument_count,
    GDExtensionVariantPtr r_return,
    GDExtensionCallError *r_error) {
    
    (void)method_userdata;
    (void)p_instance;
    
    if (r_error) {
        r_error->error = GDEXTENSION_CALL_OK;
    }
    
    if (p_argument_count != 1) {
        if (r_error) {
            r_error->error = GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS;
            r_error->argument = 1;
            r_error->expected = 1;
        }
        return;
    }
    
    GDExtensionVariantType arg_type0 = variant_get_type(p_args[0]);
    if (arg_type0 != GDEXTENSION_VARIANT_TYPE_INT) {
        if (r_error) {
            r_error->error = GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT;
        }
        return;
    }
    
    GDExtensionInt snapshot_ptr_int = 0;
    memcpy(&snapshot_ptr_int, p_args[0], sizeof(GDExtensionInt));
    
    MansionShmSnapshot *snapshot = (MansionShmSnapshot *)(uintptr_t)snapshot_ptr_int;
    uint8_t *pixels = snapshot ? mansion_gdextension_snapshot_get_pixels(snapshot) : nullptr;
    GDExtensionInt pixels_val = (GDExtensionInt)(uintptr_t)pixels;
    memcpy(r_return, &pixels_val, sizeof(GDExtensionInt));
}

/* Method: snapshot_get_pixels_size(snapshot_ptr: uint64) -> int */
static void mansion_adapter_snapshot_get_pixels_size_call(
    void *method_userdata,
    GDExtensionClassInstancePtr p_instance,
    const GDExtensionConstVariantPtr *p_args,
    GDExtensionInt p_argument_count,
    GDExtensionVariantPtr r_return,
    GDExtensionCallError *r_error) {
    
    (void)method_userdata;
    (void)p_instance;
    
    if (r_error) {
        r_error->error = GDEXTENSION_CALL_OK;
    }
    
    if (p_argument_count != 1) {
        if (r_error) {
            r_error->error = GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS;
            r_error->argument = 1;
            r_error->expected = 1;
        }
        return;
    }
    
    GDExtensionVariantType arg_type0 = variant_get_type(p_args[0]);
    if (arg_type0 != GDEXTENSION_VARIANT_TYPE_INT) {
        if (r_error) {
            r_error->error = GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT;
        }
        return;
    }
    
    GDExtensionInt snapshot_ptr_int = 0;
    memcpy(&snapshot_ptr_int, p_args[0], sizeof(GDExtensionInt));
    
    MansionShmSnapshot *snapshot = (MansionShmSnapshot *)(uintptr_t)snapshot_ptr_int;
    size_t pixels_size = snapshot ? mansion_gdextension_snapshot_get_pixels_size(snapshot) : 0;
    GDExtensionInt pixels_size_val = (GDExtensionInt)pixels_size;
    memcpy(r_return, &pixels_size_val, sizeof(GDExtensionInt));
}

/* Class registration data */
static void mansion_register_class(GDExtensionClassLibraryPtr p_library, FILE *init_log) {
    static const char class_name[] = "MansionAdapter";
    static const char parent_class[] = "RefCounted";
    
    /* Create StringName for class name and parent class name */
    GDExtensionUninitializedStringNamePtr class_name_sn_storage = nullptr;
    GDExtensionUninitializedStringNamePtr parent_name_sn_storage = nullptr;
    GDExtensionStringNamePtr class_name_sn = nullptr;
    GDExtensionStringNamePtr parent_name_sn = nullptr;
    
    /* Always initialize string names - the function is always available in Godot 4.5+ */
    string_name_new_with_latin1_chars(&class_name_sn_storage, class_name, false);
    class_name_sn = class_name_sn_storage;
    
    string_name_new_with_latin1_chars(&parent_name_sn_storage, parent_class, false);
    parent_name_sn = parent_name_sn_storage;
    
    /* Setup GDExtensionClassCreationInfo6 */
    if (init_log) fprintf(init_log, "[mansion] Setting up class info\n");
    fflush(init_log);
    
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
    
    if (init_log) fprintf(init_log, "[mansion] Class info set up\n");
    fflush(init_log);
    
    /* Register the class */
    if (classdb_register_extension_class6) {
        if (init_log) fprintf(init_log, "[mansion] Calling classdb_register_extension_class6\n");
        fflush(init_log);
        classdb_register_extension_class6(p_library, class_name_sn, parent_name_sn, &class_info);
        if (init_log) fprintf(init_log, "[mansion] classdb_register_extension_class6 returned\n");
        fflush(init_log);
    } else {
        if (init_log) fprintf(init_log, "[mansion] classdb_register_extension_class6 is null\n");
        fflush(init_log);
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
    
    /* set_compositor method */
    memset(&method_info, 0, sizeof(method_info));
    method_info.method_flags = GDEXTENSION_METHOD_FLAG_NORMAL;
    method_info.has_return_value = true;
    method_info.return_value_info = nullptr;
    method_info.return_value_metadata = GDEXTENSION_METHOD_ARGUMENT_METADATA_NONE;
    method_info.argument_count = 2;
    
    string_name_new_with_latin1_chars(&method_name_sn_storage, "set_compositor", false);
    method_info.name = method_name_sn;
    method_info.method_userdata = nullptr;
    method_info.call_func = mansion_adapter_set_compositor_call;
    method_info.ptrcall_func = nullptr;
    
    if (classdb_register_extension_class_method) {
        classdb_register_extension_class_method(p_library, class_name_sn, &method_info);
    }
    
    /* create_snapshot method */
    memset(&method_info, 0, sizeof(method_info));
    method_info.method_flags = GDEXTENSION_METHOD_FLAG_NORMAL;
    method_info.has_return_value = true;
    method_info.return_value_info = nullptr;
    method_info.return_value_metadata = GDEXTENSION_METHOD_ARGUMENT_METADATA_NONE;
    method_info.argument_count = 1;
    
    string_name_new_with_latin1_chars(&method_name_sn_storage, "create_snapshot", false);
    method_info.name = method_name_sn;
    method_info.method_userdata = nullptr;
    method_info.call_func = mansion_adapter_create_snapshot_call;
    method_info.ptrcall_func = nullptr;
    
    if (classdb_register_extension_class_method) {
        classdb_register_extension_class_method(p_library, class_name_sn, &method_info);
    }
    
    /* snapshot_destroy method */
    memset(&method_info, 0, sizeof(method_info));
    method_info.method_flags = GDEXTENSION_METHOD_FLAG_NORMAL;
    method_info.has_return_value = false;
    method_info.argument_count = 1;
    
    string_name_new_with_latin1_chars(&method_name_sn_storage, "snapshot_destroy", false);
    method_info.name = method_name_sn;
    method_info.method_userdata = nullptr;
    method_info.call_func = mansion_adapter_snapshot_destroy_call;
    method_info.ptrcall_func = nullptr;
    
    if (classdb_register_extension_class_method) {
        classdb_register_extension_class_method(p_library, class_name_sn, &method_info);
    }
    
    /* snapshot_get_width method */
    memset(&method_info, 0, sizeof(method_info));
    method_info.method_flags = GDEXTENSION_METHOD_FLAG_NORMAL;
    method_info.has_return_value = true;
    method_info.return_value_info = nullptr;
    method_info.return_value_metadata = GDEXTENSION_METHOD_ARGUMENT_METADATA_NONE;
    method_info.argument_count = 1;
    
    string_name_new_with_latin1_chars(&method_name_sn_storage, "snapshot_get_width", false);
    method_info.name = method_name_sn;
    method_info.method_userdata = nullptr;
    method_info.call_func = mansion_adapter_snapshot_get_width_call;
    method_info.ptrcall_func = nullptr;
    
    if (classdb_register_extension_class_method) {
        classdb_register_extension_class_method(p_library, class_name_sn, &method_info);
    }
    
    /* snapshot_get_height method */
    memset(&method_info, 0, sizeof(method_info));
    method_info.method_flags = GDEXTENSION_METHOD_FLAG_NORMAL;
    method_info.has_return_value = true;
    method_info.return_value_info = nullptr;
    method_info.return_value_metadata = GDEXTENSION_METHOD_ARGUMENT_METADATA_NONE;
    method_info.argument_count = 1;
    
    string_name_new_with_latin1_chars(&method_name_sn_storage, "snapshot_get_height", false);
    method_info.name = method_name_sn;
    method_info.method_userdata = nullptr;
    method_info.call_func = mansion_adapter_snapshot_get_height_call;
    method_info.ptrcall_func = nullptr;
    
    if (classdb_register_extension_class_method) {
        classdb_register_extension_class_method(p_library, class_name_sn, &method_info);
    }
    
    /* snapshot_get_stride method */
    memset(&method_info, 0, sizeof(method_info));
    method_info.method_flags = GDEXTENSION_METHOD_FLAG_NORMAL;
    method_info.has_return_value = true;
    method_info.return_value_info = nullptr;
    method_info.return_value_metadata = GDEXTENSION_METHOD_ARGUMENT_METADATA_NONE;
    method_info.argument_count = 1;
    
    string_name_new_with_latin1_chars(&method_name_sn_storage, "snapshot_get_stride", false);
    method_info.name = method_name_sn;
    method_info.method_userdata = nullptr;
    method_info.call_func = mansion_adapter_snapshot_get_stride_call;
    method_info.ptrcall_func = nullptr;
    
    if (classdb_register_extension_class_method) {
        classdb_register_extension_class_method(p_library, class_name_sn, &method_info);
    }
    
    /* snapshot_get_format method */
    memset(&method_info, 0, sizeof(method_info));
    method_info.method_flags = GDEXTENSION_METHOD_FLAG_NORMAL;
    method_info.has_return_value = true;
    method_info.return_value_info = nullptr;
    method_info.return_value_metadata = GDEXTENSION_METHOD_ARGUMENT_METADATA_NONE;
    method_info.argument_count = 1;
    
    string_name_new_with_latin1_chars(&method_name_sn_storage, "snapshot_get_format", false);
    method_info.name = method_name_sn;
    method_info.method_userdata = nullptr;
    method_info.call_func = mansion_adapter_snapshot_get_format_call;
    method_info.ptrcall_func = nullptr;
    
    if (classdb_register_extension_class_method) {
        classdb_register_extension_class_method(p_library, class_name_sn, &method_info);
    }
    
    /* snapshot_get_revision method */
    memset(&method_info, 0, sizeof(method_info));
    method_info.method_flags = GDEXTENSION_METHOD_FLAG_NORMAL;
    method_info.has_return_value = true;
    method_info.return_value_info = nullptr;
    method_info.return_value_metadata = GDEXTENSION_METHOD_ARGUMENT_METADATA_NONE;
    method_info.argument_count = 1;
    
    string_name_new_with_latin1_chars(&method_name_sn_storage, "snapshot_get_revision", false);
    method_info.name = method_name_sn;
    method_info.method_userdata = nullptr;
    method_info.call_func = mansion_adapter_snapshot_get_revision_call;
    method_info.ptrcall_func = nullptr;
    
    if (classdb_register_extension_class_method) {
        classdb_register_extension_class_method(p_library, class_name_sn, &method_info);
    }
    
    /* snapshot_get_pixels method */
    memset(&method_info, 0, sizeof(method_info));
    method_info.method_flags = GDEXTENSION_METHOD_FLAG_NORMAL;
    method_info.has_return_value = true;
    method_info.return_value_info = nullptr;
    method_info.return_value_metadata = GDEXTENSION_METHOD_ARGUMENT_METADATA_NONE;
    method_info.argument_count = 1;
    
    string_name_new_with_latin1_chars(&method_name_sn_storage, "snapshot_get_pixels", false);
    method_info.name = method_name_sn;
    method_info.method_userdata = nullptr;
    method_info.call_func = mansion_adapter_snapshot_get_pixels_call;
    method_info.ptrcall_func = nullptr;
    
    if (classdb_register_extension_class_method) {
        classdb_register_extension_class_method(p_library, class_name_sn, &method_info);
    }
    
    /* snapshot_get_pixels_size method */
    memset(&method_info, 0, sizeof(method_info));
    method_info.method_flags = GDEXTENSION_METHOD_FLAG_NORMAL;
    method_info.has_return_value = true;
    method_info.return_value_info = nullptr;
    method_info.return_value_metadata = GDEXTENSION_METHOD_ARGUMENT_METADATA_NONE;
    method_info.argument_count = 1;
    
    string_name_new_with_latin1_chars(&method_name_sn_storage, "snapshot_get_pixels_size", false);
    method_info.name = method_name_sn;
    method_info.method_userdata = nullptr;
    method_info.call_func = mansion_adapter_snapshot_get_pixels_size_call;
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
    
    FILE *init_log = fopen("/tmp/gdextension_init.log", "a");
    if (init_log) {
        fprintf(init_log, "[mansion] Initialization started\n");
        fflush(init_log);
    }
    
    /* Initialize interface function pointers */
    if (init_log) fprintf(init_log, "[mansion] Initializing interface functions\n");
    init_interface_functions(p_get_proc_address);
    
    if (init_log) {
        fprintf(init_log, "[mansion] classdb_register_extension_class6: %p\n", classdb_register_extension_class6);
        fflush(init_log);
    }
    
    if (init_log) fprintf(init_log, "[mansion] Registering class\n");
    fflush(init_log);
    
    /* Register our class */
    mansion_register_class(p_library, init_log);
    
    if (init_log) {
        fprintf(init_log, "[mansion] Class registered\n");
        fflush(init_log);
    }
    
    if (init_log) fprintf(init_log, "[mansion] Setting initialization level\n");
    /* Set initialization level */
    r_initialization->minimum_initialization_level = GDEXTENSION_INITIALIZATION_CORE;
    r_initialization->userdata = nullptr;
    r_initialization->initialize = nullptr;
    r_initialization->deinitialize = nullptr;
    
    if (init_log) {
        fprintf(init_log, "[mansion] Initialization complete\n");
        fflush(init_log);
        fclose(init_log);
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
    
    /* Log the entry point call with all parameters */
    FILE *log = fopen("/tmp/gdextension_init.log", "a");
    if (log) {
        fprintf(log, "[mansion] GDExtensionInit called (p_get_proc_address: %p, p_library: %p, r_initialization: %p)\n", 
                (void *)p_get_proc_address, (void *)p_library, (void *)r_initialization);
        fflush(log);
        fclose(log);
    }
    
    return mansion_extension_init(p_get_proc_address, p_library, r_initialization);
}

/* Mark GDExtensionExtensionDestroy with default visibility even when -fvisibility=hidden is used */
void __attribute__((visibility("default"))) GDExtensionExtensionDestroy(GDExtensionClassLibraryPtr p_library) {
    /* No cleanup needed - the extension doesn't allocate global resources */
}

} /* extern "C" */
