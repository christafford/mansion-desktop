/* Probe EGL capabilities for P2-T05 decision gate.
 * Prints available extensions and test results to stdout. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <wayland-client.h>

int main(void) {
    EGLDisplay dpy;
    EGLint major, minor;
    const char *exts;
    int found_surfaceless = 0;
    int found_wayland_bind = 0;

    /* Check EGL extensions on EGL_NO_DISPLAY */
    exts = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    printf("EGL_NO_DISPLAY extensions: %s\n", exts ? exts : "(none)");
    if (exts && strstr(exts, "EGL_MESA_platform_surfaceless"))
        found_surfaceless = 1;
    if (exts && strstr(exts, "EGL_EXT_platform_wayland"))
        found_surfaceless = 1; /* also checkable */

    /* Try surfaceless */
    if (found_surfaceless) {
        dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (dpy == EGL_NO_DISPLAY)
            dpy = eglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA, NULL, NULL);
    } else {
        dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    }

    if (dpy == EGL_NO_DISPLAY) {
        printf("FATAL: cannot get EGL display\n");
        return 1;
    }

    if (!eglInitialize(dpy, &major, &minor)) {
        printf("FATAL: eglInitialize failed\n");
        eglTerminate(dpy);
        return 1;
    }
    printf("EGL %d.%d initialized on display 0x%lx\n", major, minor, (unsigned long)dpy);

    exts = eglQueryString(dpy, EGL_EXTENSIONS);
    printf("EGL display extensions: %s\n", exts ? exts : "(none)");
    if (exts && strstr(exts, "EGL_EXT_platform_wayland"))
        found_surfaceless = 1;
    if (exts && strstr(exts, "EGL_WL_bind_wayland_display"))
        found_wayland_bind = 1;

    /* Check for EGL_WL_bind_wayland_display symbols */
    typedef EGLImageKHR (EGLAPIENTRY *egl_query_wayland_buffer_t)(EGLDisplay, EGLenum, struct wl_buffer *);
    typedef EGLBoolean (EGLAPIENTRY *egl_bind_wayland_display_t)(EGLDisplay, struct wl_display *);
    typedef EGLBoolean (EGLAPIENTRY *egl_unbind_wayland_display_t)(EGLDisplay, struct wl_display *);

    void *libegl = dlopen("libEGL.so.1", RTLD_NOW);
    if (libegl) {
        printf("libegl handle: %p\n", libegl);
        /* Try to dlsym the key functions */
        egl_bind_wayland_display_t bind_fn = (egl_bind_wayland_display_t)dlsym(libegl, "eglBindWaylandDisplayWL");
        egl_unbind_wayland_display_t unbind_fn = (egl_unbind_wayland_display_t)dlsym(libegl, "eglUnbindWaylandDisplayWL");
        egl_query_wayland_buffer_t query_fn = (egl_query_wayland_buffer_t)dlsym(libegl, "eglQueryWaylandBufferWL");
        printf("  eglBindWaylandDisplayWL: %p\n", (void*)bind_fn);
        printf("  eglUnbindWaylandDisplayWL: %p\n", (void*)unbind_fn);
        printf("  eglQueryWaylandBufferWL: %p\n", (void*)query_fn);
        dlclose(libegl);
    } else {
        printf("Cannot dlopen libEGL.so.1: %s\n", dlerror());
    }

    /* Check GL extensions - need a context */
    EGLint ctx_attr[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    EGLint num_configs;
    EGLint conf_attr[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RED_SIZE, 1, EGL_GREEN_SIZE, 1, EGL_BLUE_SIZE, 1,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_NONE
    };
    EGLConfig conf;
    if (eglChooseConfig(dpy, conf_attr, &conf, 1, &num_configs) && num_configs > 0) {
        EGLContext ctx = eglCreateContext(dpy, conf, EGL_NO_CONTEXT, ctx_attr);
        if (ctx && ctx != EGL_NO_CONTEXT) {
            EGLint pbuf_attr[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
            EGLSurface surf = eglCreatePbufferSurface(dpy, conf, pbuf_attr);
            if (surf && surf != EGL_NO_SURFACE) {
                eglMakeCurrent(dpy, surf, surf, ctx);
                printf("GL extensions: %s\n", glGetString(GL_EXTENSIONS) ? (const char*)glGetString(GL_EXTENSIONS) : "(none)");
                printf("GL vendor: %s\n", glGetString(GL_VENDOR) ? (const char*)glGetString(GL_VENDOR) : "(null)");
                printf("GL renderer: %s\n", glGetString(GL_RENDERER) ? (const char*)glGetString(GL_RENDERER) : "(null)");
                printf("GL version: %s\n", glGetString(GL_VERSION) ? (const char*)glGetString(GL_VERSION) : "(null)");
                eglDestroySurface(dpy, surf);
            }
            eglDestroyContext(dpy, ctx);
        }
    }

    eglTerminate(dpy);
    return 0;
}
