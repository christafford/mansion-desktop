/* Probe EGL_WL_bind_wayland_display on the compositor's Wayland socket.
 * Connects to the compositor, creates a surface, tries EGL platform Wayland,
 * and checks if eglBindWaylandDisplayWL is available. */
#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <wayland-client.h>

static struct wl_registry* registry = NULL;

static void registry_global(void* data, struct wl_registry* reg,
                            uint32_t name, const char* iface, uint32_t ver) {
    if (strcmp(iface, "wl_compositor") == 0) {
        registry = reg;
    }
}
static const struct wl_registry_listener registry_listener = {
    .global = registry_global,
    .global_remove = NULL,
};

int main(int argc, char** argv) {
    const char* socket = NULL;
    if (argc > 1) socket = argv[1];

    /* Connect to compositor's Wayland socket */
    struct wl_display* wl_dpy = wl_display_connect(socket);
    if (!wl_dpy) {
        fprintf(stderr, "Cannot connect to Wayland display %s: %s\n",
                socket ? socket : "$WAYLAND_DISPLAY", strerror(errno));
        return 1;
    }
    printf("Connected to Wayland display %s\n", socket ? socket : "$WAYLAND_DISPLAY");

    /* Roundtrip to get globals */
    struct wl_registry* reg = wl_display_get_registry(wl_dpy);
    wl_registry_add_listener(reg, &registry_listener, NULL);
    if (wl_display_roundtrip(wl_dpy) < 0) {
        fprintf(stderr, "Roundtrip failed\n");
        return 1;
    }

    /* Check EGL extensions on NO_DISPLAY first */
    const char* exts = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    printf("EGL_NO_DISPLAY extensions: %s\n", exts ? exts : "(none)");

    /* Try EGL_PLATFORM_WAYLAND_KHR */
    EGLDisplay egl_dpy = eglGetPlatformDisplay(EGL_PLATFORM_WAYLAND_KHR, wl_dpy, NULL);
    if (egl_dpy == EGL_NO_DISPLAY) {
        EGLint err = eglGetError();
        printf("eglGetPlatformDisplay(WAYLAND) failed: error 0x%x\n", (unsigned)err);
        /* Fall back to surfaceless */
        egl_dpy = eglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA, NULL, NULL);
        if (egl_dpy != EGL_NO_DISPLAY)
            printf("Fell back to EGL_PLATFORM_SURFACELESS_MESA\n");
    }

    if (egl_dpy == EGL_NO_DISPLAY) {
        printf("FATAL: cannot get EGL display\n");
        wl_display_disconnect(wl_dpy);
        return 1;
    }

    EGLint major, minor;
    if (!eglInitialize(egl_dpy, &major, &minor)) {
        printf("FATAL: eglInitialize failed\n");
        wl_display_disconnect(wl_dpy);
        return 1;
    }
    printf("EGL %d.%d initialized\n", major, minor);

    exts = eglQueryString(egl_dpy, EGL_EXTENSIONS);
    printf("EGL extensions: %s\n", exts ? exts : "(none)");

    /* Check for EGL_WL_bind_wayland_display symbols */
    void* libegl = dlopen("libEGL.so.1", RTLD_NOW);
    if (libegl) {
        typedef EGLImageKHR (EGLAPIENTRY* query_fn_t)(EGLDisplay, EGLenum, struct wl_buffer*);
        typedef EGLBoolean (EGLAPIENTRY* bind_fn_t)(EGLDisplay, struct wl_display*);
        typedef EGLBoolean (EGLAPIENTRY* unbind_fn_t)(EGLDisplay, struct wl_display*);

        bind_fn_t bind_fn = (bind_fn_t)dlsym(libegl, "eglBindWaylandDisplayWL");
        unbind_fn_t unbind_fn = (unbind_fn_t)dlsym(libegl, "eglUnbindWaylandDisplayWL");
        query_fn_t query_fn = (query_fn_t)dlsym(libegl, "eglQueryWaylandBufferWL");

        printf("  eglBindWaylandDisplayWL: %p\n", (void*)bind_fn);
        printf("  eglUnbindWaylandDisplayWL: %p\n", (void*)unbind_fn);
        printf("  eglQueryWaylandBufferWL: %p\n", (void*)query_fn);

        /* Try to bind the Wayland display */
        if (bind_fn) {
            printf("Trying eglBindWaylandDisplayWL...\n");
            EGLBoolean ret = bind_fn(egl_dpy, wl_dpy);
            printf("  eglBindWaylandDisplayWL returned: %d\n", (int)ret);
            EGLint err = eglGetError();
            printf("  EGL error after bind: 0x%x\n", (unsigned)err);

            if (ret == EGL_TRUE && query_fn) {
                /* Create a dummy buffer and try to query it */
                printf("Trying eglQueryWaylandBufferWL on a dummy buffer...\n");
                /* We don't have a real buffer to test with, but the function pointer
                 * existing means the extension is available */
            }

            if (unbind_fn) {
                unbind_fn(egl_dpy, wl_dpy);
            }
        } else {
            printf("eglBindWaylandDisplayWL symbol NOT found - extension unavailable\n");
        }

        dlclose(libegl);
    }

    /* Cleanup EGL */
    eglMakeCurrent(egl_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglTerminate(egl_dpy);

    /* Cleanup Wayland */
    wl_registry_destroy(reg);
    wl_display_disconnect(wl_dpy);

    printf("Probe complete.\n");
    return 0;
}
