#pragma once

#include <EGL/egl.h>
#include <wayland-server.h>

struct MansionCompositor;
struct MansionRenderer;

struct MansionDisplay {
    struct MansionCompositor* compositor;
    struct wl_display* wl_display;
    EGLDisplay egl_display;
    EGLContext egl_context;
    EGLSurface egl_surface;
    EGLConfig egl_config;
    int egl_config_count;
    int window_width;
    int window_height;
    struct MansionRenderer* renderer;
};

struct MansionDisplay* create_display(struct MansionCompositor* compositor, struct wl_display* wl_display);
/* Headless: no host window, no EGL context, no renderer. render() becomes a no-op.
 * Offscreen rendering for screenshots is a later task (docs/TASKS.md P1-T05). */
struct MansionDisplay* create_display_headless(struct MansionCompositor* compositor, struct wl_display* wl_display);
void destroy_display(struct MansionDisplay* display);
void swap_buffers(struct MansionDisplay* display);
void render(struct MansionDisplay* display);
struct MansionRenderer* get_renderer(struct MansionDisplay* display);
EGLContext get_egl_context(struct MansionDisplay* display);
bool init_renderer(struct MansionDisplay* display);
void destroy_renderer(struct MansionDisplay* display);
void render_surface(struct MansionDisplay* display, struct wl_resource* surface, int32_t x, int32_t y);
