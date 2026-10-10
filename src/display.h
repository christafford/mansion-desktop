#pragma once

#include <EGL/egl.h>
#include <wayland-server.h>
#include <wayland-client.h>
#include <wayland-egl.h>

#include "renderer-surface.h"

struct ElsewhereCompositor;
struct ElsewhereRenderer;

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

struct ElsewhereDisplay {
    struct ElsewhereCompositor* compositor;
    struct wl_display* wl_display;
    EGLDisplay egl_display;
    EGLContext egl_context;
    EGLSurface egl_surface;
    EGLConfig egl_config;
    int egl_config_count;
    int window_width;
    int window_height;
    struct ElsewhereRenderer* renderer;

    /* Wayland client connection for the host window (P5: native presentation).
     * Uses surfaceless Mesa + EGL PBuffer + wl_shm buffer export. */
    struct wl_display* wl_client_display = nullptr;
    struct wl_surface* wl_surface = nullptr;
    EGLSurface egl_pbuffer_surface = EGL_NO_SURFACE;

    /* P2-T02: 3D panel rendering state */
    Camera camera;
    Panel panel;
    bool flat_mode = false;  /* true = 2D rendering, false = 3D panel */

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

struct ElsewhereDisplay* create_display(struct ElsewhereCompositor* compositor, struct wl_display* wl_display);
/* Headless: tries EGL surfaceless Mesa for offscreen rendering; falls back to no
 * renderer if EGL is unavailable. Frame callbacks always fire. */
struct ElsewhereDisplay* create_display_headless(struct ElsewhereCompositor* compositor, struct wl_display* wl_display);
void destroy_display(struct ElsewhereDisplay* display);
void display_resize(struct ElsewhereDisplay* display);
void swap_buffers(struct ElsewhereDisplay* display);
void render(struct ElsewhereDisplay* display);
struct ElsewhereRenderer* get_renderer(struct ElsewhereDisplay* display);
EGLContext get_egl_context(struct ElsewhereDisplay* display);
bool init_renderer(struct ElsewhereDisplay* display);
void destroy_renderer(struct ElsewhereDisplay* display);
void render_surface(struct ElsewhereDisplay* display, struct wl_resource* surface, int32_t x, int32_t y);
/* Render one final frame and write the framebuffer as a binary PPM (P6) file.
 * Returns false if EGL/renderer is unavailable. */
bool take_screenshot(struct ElsewhereDisplay* display, const char* path);
/* P4-T01: log camera position to stderr (for test verification). */
void log_camera_position(struct ElsewhereDisplay* display);

/* P4-T02: teleport camera to stored viewpoint facing the monitor. */
void input_teleport(struct ElsewhereDisplay* display);

/* ─── Renderer surface accessors (P21-T10: separate protocol from presentation) ───
 *
 * The rendering layer owns GPU textures per surface.  These accessors
 * let display.cpp (and code included from it) query and update the
 * renderer-side surface state without the compositor core depending
 * on GL/EGL at all.
 *
 * get_renderer_surface() returns the rendering-layer surface struct
 * for a given protocol surface, or nullptr if no renderer surface
 * has been allocated yet.
 *
 * ensure_renderer_surface() allocates a renderer surface on demand
 * (used during surface creation).
 *
 * set_renderer_surface_texture() stores a GL texture ID on the
 * renderer surface (called after texture upload completes).
 */
struct ElsewhereRendererSurface* get_renderer_surface(
    struct ElsewhereSurface* surface);
struct ElsewhereRendererSurface* ensure_renderer_surface(
    struct ElsewhereSurface* surface);
void set_renderer_surface_texture(
    struct ElsewhereSurface* surface, GLuint texture);

/* destroy_renderer_surface() frees the GL texture and removes the
 * mapping for a surface that is being destroyed (orphaned).
 * Only called from compositor.cpp during surface destruction. */
void destroy_renderer_surface(struct ElsewhereSurface* surface);
