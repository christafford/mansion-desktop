#include <stdio.h>
#include <dlfcn.h>

typedef int (*GDExtensionInitFunc)(void *p_get_proc_address, void *p_library, void *r_initialization);

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s <gdextension_library.so>\n", argv[0]);
        return 1;
    }
    
    printf("Loading GDExtension library: %s\n", argv[1]);
    fflush(stdout);
    
    void *handle = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        fprintf(stderr, "Failed to load library: %s\n", dlerror());
        return 1;
    }
    
    printf("Library loaded successfully!\n");
    fflush(stdout);
    
    GDExtensionInitFunc init_func = (GDExtensionInitFunc)dlsym(handle, "GDExtensionInit");
    if (!init_func) {
        fprintf(stderr, "Failed to find GDExtensionInit: %s\n", dlerror());
        dlclose(handle);
        return 1;
    }
    
    printf("GDExtensionInit found at %p\n", (void*)init_func);
    fflush(stdout);
    
    // Don't actually call it - just verify we can load it
    printf("GDExtension library is valid and can be loaded!\n");
    
    dlclose(handle);
    return 0;
}
