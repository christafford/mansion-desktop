#pragma once

#include <EGL/egl.h>
#include <wayland-server.h>
#include <wayland-client.h>
#include <wayland-egl.h>

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

    /* Wayland client connection for the host window (P5: native presentation).
     * All pointers default to null; only set in windowed mode. */
    struct wl_display* wl_client_display = nullptr;
    struct wl_surface* wl_surface = nullptr;
    struct wl_egl_window* wl_egl_window = nullptr;

    /* P2-T02: 3D panel rendering state */
    Camera camera;
    Panel panel;
    bool flat_mode = false;  /* true = 2D rendering, false = 3D panel */

    /* P2-T05: EGL mode — buffers are RGBA (no swizzle needed) */
    bool egl_mode = false;

    /* P4-T01: Room/world mode — when true the room geometry is drawn
     * as background before the 3D panel. */
    bool room_mode = false;

    /* P2-T07: frame timing / stats (accumulated, printed every 60 frames) */
    bool stats_enabled = false;
    long long stats_frame_count = 0;       /* frames since last print */
    long long stats_total_render_us = 0;   /* total render time (us) */
    long long stats_total_bytes_up = 0;    /* total bytes uploaded */

    /* P3-T04: flat-mode state for full-size application presentation.
     * When entering Application mode, flat_mode is set to true and
     * the focused surface is drawn fullscreen.  flat_mode_prev
     * remembers the previous value so it can be restored. */
    bool flat_mode_prev = false;

    /* P3-T05: panel targeting state. */
    bool panel_targeted = true;  /* ray from screen centre always hits panel */

    /* P4-T02: teleport target position and orientation. */
    float teleport_x = 0, teleport_y = 0, teleport_z = 0;
    float teleport_yaw = 0, teleport_pitch = 0;
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
/* P4-T01: log camera position to stderr (for test verification). */
void log_camera_position(struct MansionDisplay* display);

/* P4-T02: teleport camera to stored viewpoint facing the monitor. */
void input_teleport(struct MansionDisplay* display);
