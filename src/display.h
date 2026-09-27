#pragma once

#include <EGL/egl.h>
#include <wayland-server.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

struct MansionCompositor;
struct MansionRenderer;

// Camera configuration (P2-T02)
struct Camera {
    float x = 0, y = 0, z = 10;
    float yaw = 0;     // rotation around Y axis (horizontal)
    float pitch = 0;   // rotation around X axis (vertical)
    float fov = 1.5708f;  // PI/2 = 90 degrees
};

// Panel geometry for 3D rendering (P2-T02)
struct Panel {
    float x = 0, y = 0, z = 0;    // center position
    float width = 3.0f;             // size in world units
    float height = 2.0f;
    float nx = 0, ny = 0, nz = -1; // normal (facing camera by default)
};

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
    Display* x_display;      /* X11 display (windowed mode only) */
    Window x_window;         /* X11 window (windowed mode only) */

    /* P2-T02: 3D panel rendering state */
    Camera camera;
    Panel panel;
    bool flat_mode = false;  /* true = 2D rendering, false = 3D panel */

    /* P2-T05: EGL mode — buffers are RGBA (no swizzle needed) */
    bool egl_mode = false;
};

struct MansionDisplay* create_display(struct MansionCompositor* compositor, struct wl_display* wl_display);
/* Headless: tries EGL surfaceless Mesa for offscreen rendering; falls back to no
 * renderer if EGL is unavailable. Frame callbacks always fire. */
struct MansionDisplay* create_display_headless(struct MansionCompositor* compositor, struct wl_display* wl_display);
void destroy_display(struct MansionDisplay* display);
void display_resize(struct MansionDisplay* display);
void swap_buffers(struct MansionDisplay* display);
void render(struct MansionDisplay* display);
struct MansionRenderer* get_renderer(struct MansionDisplay* display);
EGLContext get_egl_context(struct MansionDisplay* display);
bool init_renderer(struct MansionDisplay* display);
void destroy_renderer(struct MansionDisplay* display);
void render_surface(struct MansionDisplay* display, struct wl_resource* surface, int32_t x, int32_t y);
/* Render one final frame and write the framebuffer as a binary PPM (P6) file.
 * Returns false if EGL/renderer is unavailable. */
bool take_screenshot(struct MansionDisplay* display, const char* path);
