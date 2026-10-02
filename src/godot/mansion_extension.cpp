/*
 * MansionExtension — GDExtension entry point for Mansion Desktop.
 *
 * This file provides the GDExtension API symbols that Godot requires
 * to load and unload the extension. It wraps the C-facing MansionBridge
 * and exposes a minimal Godot-native API.
 *
 * The extension creates a private Wayland compositor on init and
 * pumps events from the main thread. No rendering or input device
 * scanning is done by the extension — those are handled by the
 * Godot-side bridge in GDScript (P21-T12+).
 */

/* GDExtension API header (minimal, no godot-cpp dependency) */
#include <gdextension_interface.h>

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <string>
#include <vector>
#include <functional>

#include <wayland-server.h>

#include "compositor.h"
#include "compositor-private.h"
#include "xdg-shell.h"
#include "input.h"
#include "launch.h"
#include "mansion_bridge.h"

/* ─── GDExtension library handle (set by Godot at init) ─── */

static GDExtensionInterfaceGetProcAddress c_interface = nullptr;
static void *library = nullptr;

/* ─── MansionAdapterWrapper — GDExtension-visible adapter ─── */
/* Renamed from MansionCompositor to avoid conflict with
 * the struct MansionCompositor defined in compositor-private.h */

class MansionAdapterWrapper {
public:
    void *adapter = nullptr;
    bool initialized = false;

    /* Configuration */
    std::string socket_name;
    int configure_width = 800;
    int configure_height = 600;
    bool debug_logging = false;

    /* Surface state (cached from last enumeration) */
    struct SurfaceEntry {
        uint32_t serial;
        int32_t width, height, x, y;
        bool needs_upload;
    };
    std::vector<SurfaceEntry> surfaces;

    /* ─── C callback for surface enumeration ─── */
    static void enumerate_callback(void *user_data, uint32_t serial,
                                    const MansionSurfaceInfo *info) {
        auto *comp = static_cast<MansionAdapterWrapper *>(user_data);
        if (!comp || !info) return;

        SurfaceEntry entry;
        entry.serial = serial;
        entry.width = info->width;
        entry.height = info->height;
        entry.x = info->x;
        entry.y = info->y;
        entry.needs_upload = info->needs_upload;

        comp->surfaces.push_back(entry);
    }

    /* ─── Lifecycle ─── */

    bool initialize() {
        if (initialized) return true;

        MansionAdapterConfig config;
        config.socket_name = socket_name.empty() ? nullptr : socket_name.c_str();
        config.configure_width = configure_width;
        config.configure_height = configure_height;
        config.debug_logging = debug_logging;

        adapter = mansion_adapter_create(&config);
        initialized = (adapter != nullptr);

        if (adapter && debug_logging) {
            fprintf(stderr, "[mansion] adapter created\n");
        }

        return initialized;
    }

    void do_shutdown() {
        if (!initialized) return;
        mansion_adapter_shutdown(adapter);
    }

    void destroy() {
        if (!initialized) return;
        mansion_adapter_destroy(adapter);
        adapter = nullptr;
        initialized = false;
        surfaces.clear();
    }

    /* ─── Event pump ─── */

    int pump() {
        if (!initialized) return -1;
        return mansion_adapter_pump(adapter);
    }

    int flush() {
        if (!initialized) return -1;
        return mansion_adapter_flush(adapter);
    }

    /* ─── Surface enumeration ─── */

    int enumerate_surfaces() {
        if (!initialized) return -1;
        surfaces.clear();
        mansion_adapter_enumerate_surfaces(adapter, enumerate_callback, this);
        return static_cast<int>(surfaces.size());
    }

    /* ─── Frame callbacks ─── */

    int fire_frame_callbacks() {
        if (!initialized) return 0;
        return mansion_adapter_fire_frame_callbacks(adapter);
    }

    /* ─── Client launch ─── */

    int32_t launch_client(const char *command) {
        if (!initialized || !command) return -1;
        return mansion_adapter_launch_client(adapter, command);
    }

    /* ─── Focus ─── */

    uint32_t get_focused_serial() const {
        if (!initialized) return 0;
        return mansion_adapter_get_focused_serial(adapter);
    }

    MansionAdapterWrapper() {}
    ~MansionAdapterWrapper() { destroy(); }
};

/* ─── GDExtension init/deinit ─── */

extern "C" {

/* GDExtension initialization — called by Godot when loading the extension.
 * This function is exported via the library's symbol table. */

GDExtensionBool godot_gdnative_init(
    GDExtensionInterfaceGetProcAddress p_get_proc_address,
    GDExtensionClassLibraryPtr p_library,
    GDExtensionInitialization *r_initialization) {

    c_interface = p_get_proc_address;
    library = p_library;

    r_initialization->minimum_initialization_level = GDEXTENSION_INITIALIZATION_CORE;
    r_initialization->userdata = nullptr;
    r_initialization->initialize = [](void *p_userdata, GDExtensionInitializationLevel p_level) {
        (void)p_userdata;
        (void)p_level;
    };
    r_initialization->deinitialize = [](void *p_userdata, GDExtensionInitializationLevel p_level) {
        (void)p_userdata;
        (void)p_level;
    };

    return true;
}

void godot_gdnative_exit(void *token) {
    (void)token;
}

/* GDExtension symbol lookup — Godot calls this to find the init function. */
void *godot_extension_get_library_symbol(const char *symbol_name) {
    if (strcmp(symbol_name, "godot_gdnative_init") == 0) {
        return reinterpret_cast<void *>(godot_gdnative_init);
    }
    if (strcmp(symbol_name, "godot_gdnative_exit") == 0) {
        return reinterpret_cast<void *>(godot_gdnative_exit);
    }
    return nullptr;
}

} /* extern "C" */

/*
 * Compilation modes:
 *
 * 1. Bridge only (mansion_bridge.cpp): Compiles against the mansion core.
 *    Verifies the C-facing bridge API works correctly.
 *    No GDExtension dependencies required.
 *
 * 2. Full extension (mansion_extension.cpp + mansion_bridge.cpp):
 *    Compiled as a shared library with godot-cpp and libgodot.so.
 *    Produces a loadable .so for Godot.
 *    Requires libgodot.so (from a full Godot engine build).
 *
 * The GDExtension entry point above uses the raw GDExtension interface
 * (gdextension_interface.h) for maximum portability. When libgodot.so
 * and godot-cpp are available, full class registration with Godot's
 * type system can be added.
 */
