/* Probe EGL + DMA-BUF capabilities. Tests PBuffer rendering and checks
 * for EGL_MESA_image_dma_buf_export support. */
#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>

int main(void) {
    printf("=== EGL + DMA-BUF Probe ===\n");

    /* 1. Initialize EGL surfaceless */
    EGLDisplay dpy = eglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA, NULL, NULL);
    if (dpy == EGL_NO_DISPLAY) {
        printf("FATAL: eglGetPlatformDisplay failed\n");
        return 1;
    }
    EGLint major, minor;
    if (!eglInitialize(dpy, &major, &minor)) {
        printf("FATAL: eglInitialize failed\n");
        return 1;
    }
    printf("EGL %d.%d initialized\n", major, minor);

    const char* exts = eglQueryString(dpy, EGL_EXTENSIONS);
    printf("EGL extensions: %s\n", exts ? exts : "(none)");

    /* Check for DMA-BUF related extensions */
    printf("\n--- DMA-BUF Extension Check ---\n");
    if (exts) {
        printf("EGL_EXT_image_dma_buf_import: %s\n",
               strstr(exts, "EGL_EXT_image_dma_buf_import") ? "YES" : "NO");
        printf("EGL_EXT_image_dma_buf_import_modifiers: %s\n",
               strstr(exts, "EGL_EXT_image_dma_buf_import_modifiers") ? "YES" : "NO");
        printf("EGL_MESA_image_dma_buf_export: %s\n",
               strstr(exts, "EGL_MESA_image_dma_buf_export") ? "YES" : "NO");
        printf("EGL_KHR_image: %s\n",
               strstr(exts, "EGL_KHR_image") ? "YES" : "NO");
    }

    /* 2. Create PBuffer and render green */
    printf("\n--- PBuffer Rendering Test ---\n");
    EGLint config_attr[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_NONE
    };
    EGLConfig config;
    EGLint num_configs = 0;
    if (!eglChooseConfig(dpy, config_attr, &config, 1, &num_configs)) {
        printf("Failed to choose config\n");
        return 1;
    }

    EGLint pbuf_attr[] = { EGL_WIDTH, 256, EGL_HEIGHT, 256, EGL_NONE };
    EGLSurface surf = eglCreatePbufferSurface(dpy, config, pbuf_attr);
    if (surf == EGL_NO_SURFACE) {
        printf("PBuffer creation failed\n");
        return 1;
    }

    EGLint ctx_attr[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    EGLContext ctx = eglCreateContext(dpy, config, EGL_NO_CONTEXT, ctx_attr);
    if (ctx == EGL_NO_CONTEXT) {
        printf("Context creation failed\n");
        return 1;
    }

    if (eglMakeCurrent(dpy, surf, surf, ctx) == EGL_FALSE) {
        printf("Make current failed\n");
        return 1;
    }
    printf("EGL context current on PBuffer\n");

    /* Render green */
    glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    printf("Rendered green screen (0,1,0)\n");

    /* Read back */
    unsigned char pixels[256 * 256 * 4];
    glReadPixels(0, 0, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    int r = pixels[4], g = pixels[5], b = pixels[6];
    printf("Read pixel at (1,1): R=%d G=%d B=%d\n", r, g, b);
    printf("GPU rendering %s\n", (r == 0 && g == 255 && b == 0) ? "WORKS" : "MAY BE WRONG");

    /* 3. Check EGL_MESA_image_dma_buf_export symbols */
    printf("\n--- DMA-BUF Export Symbol Check ---\n");
    void* libegl = dlopen("libEGL.so.1", RTLD_NOW);
    if (libegl) {
        typedef EGLImageKHR (EGLAPIENTRY* eci_t)(EGLDisplay, EGLContext, EGLenum, EGLClientBuffer, const EGLint*);
        typedef EGLBoolean (EGLAPIENTRY* eqi_t)(EGLDisplay, EGLImageKHR, EGLenum, EGLint*);
        typedef EGLBoolean (EGLAPIENTRY* dci_t)(EGLDisplay, EGLImageKHR);

        eci_t eci = (eci_t)dlsym(libegl, "eglCreateImageKHR");
        eqi_t eqi = (eqi_t)dlsym(libegl, "eglQueryImageKHR");
        dci_t dci = (dci_t)dlsym(libegl, "eglDestroyImageKHR");

        printf("eglCreateImageKHR: %p\n", (void*)eci);
        printf("eglQueryImageKHR:  %p\n", (void*)eqi);
        printf("eglDestroyImageKHR:%p\n", (void*)dci);

        /* Try to export the PBuffer as an EGL image with DMA-BUF */
        if (eci && eqi && dci && exts && strstr(exts, "EGL_MESA_image_dma_buf_export")) {
            printf("\n--- Trying DMA-BUF export ---\n");

            /* First create an EGL image from the PBuffer surface */
            EGLint img_attrs[] = {
                EGL_WIDTH, 256, EGL_HEIGHT, 256, EGL_NONE
            };
            EGLImageKHR img = eci(dpy, ctx, EGL_GL_TEXTURE_2D,
                                   (EGLClientBuffer)EGL_NO_TEXTURE, img_attrs);
            if (img && img != EGL_NO_IMAGE_KHR) {
                printf("EGLImage created\n");

                /* Try to query DMA-BUF attributes */
                EGLint attr_list[128];
                memset(attr_list, 0, sizeof(attr_list));
                EGLBoolean ret = eqi(dpy, img, EGL_LINUX_DMA_BUF_EXT, attr_list);
                printf("eglQueryImageKHR(EGL_LINUX_DMA_BUF_EXT): %d\n", (int)ret);

                if (ret) {
                    for (int i = 0; attr_list[i] != EGL_NONE; i += 2) {
                        printf("  0x%x = %d\n", attr_list[i], attr_list[i+1]);
                    }
                } else {
                    printf("  Error: 0x%x\n", (unsigned)eglGetError());
                }

                dci(dpy, img);
            } else {
                printf("EGLImage creation failed: 0x%x\n", (unsigned)eglGetError());
            }
        } else {
            printf("DMA-BUF export: %s\n",
                   exts && strstr(exts, "EGL_MESA_image_dma_buf_export") ?
                   "extension available, symbols present" :
                   exts ? "extension missing" : "no extensions");
        }

        dlclose(libegl);
    }

    /* Cleanup */
    eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(dpy, ctx);
    eglDestroySurface(dpy, surf);
    eglTerminate(dpy);

    printf("\n=== Probe complete ===\n");
    return 0;
}
