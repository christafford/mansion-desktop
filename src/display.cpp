#include <cstring>
#include <cmath>
#include <chrono>
#include <iostream>
#include <string>
#include <vector>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <wayland-client.h>
#include <wayland-egl.h>
#include <wayland-server-protocol.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "display.h"
#include "compositor.h"
#include "compositor-private.h"
#include "math.h"
#include "room.h"
#include "xdg-shell.h"
#include "input.h"

/* ─── MansionRenderer definition ───────────────────────────────────────────── */

struct MansionRenderer {
    GLuint program;
    GLint pos_attrib;
    GLint tex_attrib;
    GLint color_uniform;
    GLint tex_uniform;
    GLint viewport_uniform;

    /* P2-T02: 3D panel rendering */
    GLuint program_3d;
    GLint pos_3d;
    GLint tex_3d;          /* attribute for texcoord */
    GLint mvp_uniform;
    GLint color_3d_uniform; /* color uniform */
    GLint tex_3d_uniform;   /* sampler2D uniform */

    /* P2-T07: bytes uploaded since last stats print (cumulative) */
    long long bytes_uploaded = 0;
};

/* Generated xdg-shell client protocol (interfaces, functions, listener types). */
#include "xdg-shell-client-protocol.h"

/* Generated relative-pointer client protocol. */
#include "relative-pointer-client-protocol.h"

/* EGL attribute arrays. */
static const EGLint config_attr[] = {
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_ALPHA_SIZE, 8,
    EGL_DEPTH_SIZE, 0,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_NONE
};

static const EGLint context_attr[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
};

/* ─── Forward declarations ─────────────────────────────────────────────────── */

static GLuint create_shader(GLenum type, const char* source);
static GLuint create_program(const char* vertex_shader_source,
                             const char* fragment_shader_source);
static void render_application_fullscreen(struct MansionDisplay* display);

/* ─── Shader / program helpers ──────────────────────────────────────────────── */

static GLuint create_shader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        GLint length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        std::string log(length + 1, '\0');
        glGetShaderInfoLog(shader, length, nullptr, &log[0]);
        std::cerr << "Shader compilation failed: " << log << std::endl;
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

static GLuint create_program(const char* vertex_shader_source,
                             const char* fragment_shader_source) {
    GLuint vertex_shader = create_shader(GL_VERTEX_SHADER, vertex_shader_source);
    if (!vertex_shader) return 0;

    GLuint fragment_shader =
        create_shader(GL_FRAGMENT_SHADER, fragment_shader_source);
    if (!fragment_shader) {
        glDeleteShader(vertex_shader);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    GLint link_status;
    glGetProgramiv(program, GL_LINK_STATUS, &link_status);
    if (link_status == GL_FALSE) {
        GLint length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        std::string log(length + 1, '\0');
        glGetProgramInfoLog(program, length, nullptr, &log[0]);
        std::cerr << "Shader link failed: " << log << std::endl;
        glDeleteProgram(program);
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        return 0;
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    return program;
}

/* ─── State for the xdg_shell client setup ─────────────────────────────────── */

struct host_window_state {
    struct wl_display *wl_client;       /* Wayland client display */
    struct wl_registry *registry;       /* registry for global enumeration */
    struct xdg_wm_base *wm_base;        /* xdg_wm_base global */
    struct wl_compositor *compositor;   /* wl_compositor global */
    struct wl_shm *shm;                 /* wl_shm for buffer export */
    struct wl_seat *seat;               /* wl_seat for input */
    struct wl_keyboard *keyboard;       /* wl_keyboard resource */
    struct wl_pointer *pointer;         /* wl_pointer resource */
    struct zwp_relative_pointer_manager_v1 *rel_ptr_mgr;
    struct zwp_relative_pointer_v1 *rel_ptr;
    struct wl_surface *surface;         /* Wayland surface */
    struct xdg_surface *xdg_surface;    /* xdg_surface */
    struct xdg_toplevel *toplevel;      /* xdg_toplevel */
    uint32_t configure_serial;          /* serial for ack_configure */
    int width;
    int height;
    bool configured;
    struct MansionDisplay *display;     /* back-pointer for size sync */
};

/* ─── wl_registry global ───────────────────────────────────────────────────── */

static struct host_window_state *g_hws = nullptr;

static void registry_global(void *data, struct wl_registry *registry,
                            uint32_t id, const char *interface,
                            uint32_t version)
{
    (void)registry;
    struct host_window_state *hws = (struct host_window_state *)data;
    if (!hws) return;
    if (strcmp(interface, "xdg_wm_base") == 0 && !hws->wm_base) {
        hws->wm_base =
            (struct xdg_wm_base *)wl_registry_bind(registry, id,
                                                   &xdg_wm_base_interface,
                                                   1);
    }
    if (strcmp(interface, "wl_compositor") == 0 && g_hws &&
        !hws->compositor) {
        hws->compositor =
            (struct wl_compositor *)wl_registry_bind(registry, id,
                                                     &wl_compositor_interface,
                                                     1);
    }
    if (strcmp(interface, "wl_shm") == 0 && !hws->shm) {
        hws->shm =
            (struct wl_shm *)wl_registry_bind(registry, id,
                                               &wl_shm_interface, 1);
    }
    if (strcmp(interface, "wl_seat") == 0 && !hws->seat &&
        version >= 7) {
        hws->seat =
            (struct wl_seat *)wl_registry_bind(registry, id,
                                                &wl_seat_interface, 7);
    }
    if (strcmp(interface, "zwp_relative_pointer_manager_v1") == 0 &&
        !hws->rel_ptr_mgr) {
        hws->rel_ptr_mgr =
            (struct zwp_relative_pointer_manager_v1 *)
                wl_registry_bind(registry, id,
                                 &zwp_relative_pointer_manager_v1_interface,
                                 1);
    }
}

static void registry_global_remove(void *data, struct wl_registry *registry,
                                   uint32_t name)
{
    (void)data; (void)registry; (void)name;
}

static constexpr struct wl_registry_listener registry_listener = {
    .global = registry_global,
    .global_remove = registry_global_remove,
};

/* ─── xdg_shell listeners ──────────────────────────────────────────────────── */

static void wm_base_ping(void *data, struct xdg_wm_base *wm, uint32_t serial)
{
    (void)wm;
    xdg_wm_base_pong(wm, serial);
}

static constexpr struct xdg_wm_base_listener wm_base_listener = {
    .ping = wm_base_ping,
};

static void xdg_surface_configure(void *data, struct xdg_surface *surface,
                                  uint32_t serial)
{
    (void)surface;
    xdg_surface_ack_configure(surface, serial);
    g_hws->configure_serial = serial;
    g_hws->configured = true;
}

static void toplevel_configure_bounds(void *data, struct xdg_toplevel *toplevel,
                                       int32_t width, int32_t height)
{
    (void)data; (void)toplevel; (void)width; (void)height;
}

static void toplevel_wm_capabilities(void *data, struct xdg_toplevel *toplevel,
                                      struct wl_array *capabilities)
{
    (void)data; (void)toplevel; (void)capabilities;
}

static constexpr struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure,
};

static void toplevel_close(void *data, struct xdg_toplevel *toplevel)
{
    (void)toplevel; (void)data;
    /* Request the main loop to exit — set a flag that main.cpp checks. */
    input_handle_window_close();
}

static void toplevel_configure(void *data, struct xdg_toplevel *toplevel,
                               int32_t width, int32_t height,
                               struct wl_array *states)
{
    (void)toplevel; (void)states;
    struct host_window_state *hws = (struct host_window_state *)data;
    if (width <= 0 || height <= 0) return;

    hws->width  = width;
    hws->height = height;

    /* Sync the MansionDisplay dimensions so the renderer uses the
     * compositor-reported size instead of the startup default. */
    if (hws->display) {
        hws->display->window_width  = width;
        hws->display->window_height = height;
    }
}

static constexpr struct xdg_toplevel_listener toplevel_listener = {
    .configure          = toplevel_configure,
    .close              = toplevel_close,
    .configure_bounds   = toplevel_configure_bounds,
    .wm_capabilities    = toplevel_wm_capabilities,
};

/* ─── wl_keyboard event listener ──────────────────────────────────────────── */

static void keyboard_keymap(void *data, struct wl_keyboard *keyboard,
                            uint32_t format, int32_t fd, uint32_t size)
{
    (void)data; (void)keyboard; (void)format; (void)fd; (void)size;
    /* No XKB keymap parsing — Mansion Desktop maps WASD manually. */
}

static void keyboard_enter(void *data, struct wl_keyboard *keyboard,
                           uint32_t serial, struct wl_surface *surface,
                           struct wl_array *keys)
{
    (void)data; (void)keyboard; (void)serial; (void)surface;
    (void)keys;
}

static void keyboard_leave(void *data, struct wl_keyboard *keyboard,
                           uint32_t serial, struct wl_surface *surface)
{
    (void)data; (void)keyboard; (void)serial; (void)surface;
}

static void keyboard_key(void *data, struct wl_keyboard *keyboard,
                         uint32_t serial, uint32_t time, uint32_t key,
                         uint32_t state)
{
    (void)data; (void)keyboard; (void)serial; (void)time;
    /* Map keys to WASD: w=26, a=38, s=39, d=40 (Linux keycode). */
    bool w = false, a = false, s = false, d = false;
    if (key == 26 && state == WL_KEYBOARD_KEY_STATE_PRESSED) w = true;
    if (key == 38 && state == WL_KEYBOARD_KEY_STATE_PRESSED) a = true;
    if (key == 39 && state == WL_KEYBOARD_KEY_STATE_PRESSED) s = true;
    if (key == 40 && state == WL_KEYBOARD_KEY_STATE_PRESSED) d = true;
    input_wayland_key(w, a, s, d);
}

static void keyboard_modifiers(void *data, struct wl_keyboard *keyboard,
                               uint32_t serial, uint32_t mods_depressed,
                               uint32_t mods_latched, uint32_t mods_locked,
                               uint32_t group)
{
    (void)data; (void)keyboard; (void)serial;
    (void)mods_depressed; (void)mods_latched; (void)mods_locked;
    (void)group;
}

static void keyboard_repeat_info(void *data, struct wl_keyboard *keyboard,
                                  int32_t rate, int32_t delay)
{
    (void)data; (void)keyboard; (void)rate; (void)delay;
}

static constexpr struct wl_keyboard_listener keyboard_listener = {
    .keymap        = keyboard_keymap,
    .enter         = keyboard_enter,
    .leave         = keyboard_leave,
    .key           = keyboard_key,
    .modifiers     = keyboard_modifiers,
    .repeat_info   = keyboard_repeat_info,
};

/* ─── wl_pointer event listener ─────────────────────────────────────────────── */

static void pointer_enter(void *data, struct wl_pointer *pointer,
                          uint32_t serial, struct wl_surface *surface,
                          wl_fixed_t surface_x, wl_fixed_t surface_y)
{ (void)data; (void)pointer; (void)serial; (void)surface;
  (void)surface_x; (void)surface_y; }

static void pointer_leave(void *data, struct wl_pointer *pointer,
                          uint32_t serial, struct wl_surface *surface)
{ (void)data; (void)pointer; (void)serial; (void)surface; }

static void pointer_motion(void *data, struct wl_pointer *pointer,
                           uint32_t time, wl_fixed_t surface_x,
                           wl_fixed_t surface_y)
{
    (void)pointer; (void)time; (void)surface_x; (void)surface_y;
}

static void pointer_button(void *data, struct wl_pointer *pointer,
                           uint32_t serial, uint32_t time, uint32_t button,
                           uint32_t state)
{ (void)data; (void)pointer; (void)serial; (void)time;
  (void)button; (void)state; }

static void pointer_axis(void *data, struct wl_pointer *pointer,
                         uint32_t time, uint32_t axis, wl_fixed_t value)
{ (void)data; (void)pointer; (void)time; (void)axis; (void)value; }

static void pointer_frame(void *data, struct wl_pointer *pointer)
{ (void)data; (void)pointer; }

static void pointer_axis_source(void *data, struct wl_pointer *pointer,
                                 uint32_t source)
{ (void)data; (void)pointer; (void)source; }

static void pointer_axis_stop(void *data, struct wl_pointer *pointer,
                               uint32_t time, uint32_t axis)
{ (void)data; (void)pointer; (void)time; (void)axis; }

static void pointer_axis_discrete(void *data, struct wl_pointer *pointer,
                                   uint32_t axis, int32_t discrete)
{ (void)data; (void)pointer; (void)axis; (void)discrete; }

static void pointer_axis_value120(void *data, struct wl_pointer *pointer,
                                   uint32_t axis, int32_t value120)
{ (void)data; (void)pointer; (void)axis; (void)value120; }

static void pointer_axis_relative_direction(
    void *data, struct wl_pointer *pointer, uint32_t axis,
    uint32_t wl_pointer_axis_relative_direction)
{ (void)data; (void)pointer; (void)axis; (void)wl_pointer_axis_relative_direction; }

static constexpr struct wl_pointer_listener pointer_listener = {
    .enter                   = pointer_enter,
    .leave                   = pointer_leave,
    .motion                  = pointer_motion,
    .button                  = pointer_button,
    .axis                    = pointer_axis,
    .frame                   = pointer_frame,
    .axis_source             = pointer_axis_source,
    .axis_stop               = pointer_axis_stop,
    .axis_discrete           = pointer_axis_discrete,
    .axis_value120           = pointer_axis_value120,
    .axis_relative_direction = pointer_axis_relative_direction,
    .warp                    = nullptr,
};

/* ─── zwp_relative_pointer event listener ──────────────────────────────────── */

static void relative_pointer_motion(void *data,
                                     struct zwp_relative_pointer_v1 *rel_ptr,
                                     uint32_t time_hi, uint32_t time_lo,
                                     wl_fixed_t dx, wl_fixed_t dy,
                                     wl_fixed_t dx_unaccel, wl_fixed_t dy_unaccel)
{
    (void)data; (void)rel_ptr; (void)time_hi; (void)time_lo;
    int32_t ddx = wl_fixed_to_int(dx_unaccel);
    int32_t ddy = wl_fixed_to_int(dy_unaccel);
    if (ddx != 0 || ddy != 0) {
        input_wayland_pointer_motion(ddx, ddy);
    }
}

static constexpr struct zwp_relative_pointer_v1_listener relative_pointer_listener = {
    .relative_motion = relative_pointer_motion,
};

/* ─── wl_seat capabilities handler ───────────────────────────────────────────
 *
 * Called by the compositor when seat capabilities change (hotplug, focus
 * changes, etc.).  This replaces the nullptr that caused KWin to suppress
 * pointer and keyboard events.
 */
static void seat_capabilities(void *data, struct wl_seat *seat,
                              uint32_t capabilities)
{
    struct host_window_state *hws =
        (struct host_window_state *)data;
    if (capabilities & WL_SEAT_CAPABILITY_POINTER && !hws->pointer) {
        hws->pointer = (struct wl_pointer *)
            wl_seat_get_pointer(seat);
        if (hws->pointer) {
            wl_proxy_add_listener(
                (struct wl_proxy *)hws->pointer,
                (void (**)(void))&pointer_listener,
                nullptr);
            if (hws->rel_ptr_mgr) {
                hws->rel_ptr =
                    (struct zwp_relative_pointer_v1 *)
                    zwp_relative_pointer_manager_v1_get_relative_pointer(
                        hws->rel_ptr_mgr, hws->pointer);
                if (hws->rel_ptr) {
                    wl_proxy_add_listener(
                        (struct wl_proxy *)hws->rel_ptr,
                        (void (**)(void))
                            &relative_pointer_listener,
                        nullptr);
                }
            }
        }
    }
    if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD && !hws->keyboard) {
        hws->keyboard = (struct wl_keyboard *)
            wl_seat_get_keyboard(seat);
        if (hws->keyboard) {
            wl_proxy_add_listener(
                (struct wl_proxy *)hws->keyboard,
                (void (**)(void))&keyboard_listener,
                nullptr);
        }
    }
}

/* ─── Wayland client display setup ───────────────────────────────────────────
 *
 * Creates a native Wayland client connection to the running compositor
 * (KWin on Wayland). This eliminates the X11 → Xwayland → KWin chain that
 * caused flickering: the EGL frame is now presented directly by KWin
 * as the compositor's own output, with proper vsync.
 *
 *   Before (flickering):
 *     Our EGL → X11 window → Xwayland → KWin → Screen
 *
 *   After (smooth):
 *     Our EGL (PBuffer) → wl_shm buffer → KWin (compositor) → Screen
 *
 * Uses surfaceless Mesa for EGL rendering and wl_shm buffer export
 * for frame presentation to the compositor.
 */

bool setup_wayland_client_window(struct MansionDisplay* display)
{
    /* Connect to the Wayland compositor (reads WAYLAND_DISPLAY env var). */
    struct wl_display *wl_client = wl_display_connect(nullptr);
    if (!wl_client) {
        std::cerr << "Failed to connect to Wayland compositor" << std::endl;
        return false;
    }

    /* Allocate state for this display. */
    struct host_window_state *hws = new host_window_state{};
    hws->wl_client  = wl_client;
    hws->display    = display;
    hws->width      = display->window_width;
    hws->height     = display->window_height;

    /* Register to receive Wayland globals (compositor, shm, seat, etc). */
    g_hws = hws;
    hws->registry = wl_display_get_registry(wl_client);
    wl_registry_add_listener(hws->registry, &registry_listener, hws);
    wl_display_roundtrip(wl_client);

    if (!hws->wm_base) {
        std::cerr << "xdg_wm_base not available" << std::endl;
        wl_display_disconnect(wl_client);
        delete hws;
        return false;
    }

    /* Create the Wayland surface. */
    if (!hws->compositor) {
        std::cerr << "wl_compositor not available" << std::endl;
        wl_display_disconnect(wl_client);
        delete hws;
        return false;
    }
    hws->surface = wl_compositor_create_surface(hws->compositor);

    /* Create the xdg_surface + xdg_toplevel. */
    hws->xdg_surface = xdg_wm_base_get_xdg_surface(hws->wm_base, hws->surface);
    wl_proxy_add_listener((struct wl_proxy *)hws->xdg_surface,
                          (void (**)(void))&xdg_surface_listener, nullptr);

    hws->toplevel = (struct xdg_toplevel *)
        wl_proxy_marshal_constructor((struct wl_proxy *)hws->xdg_surface,
                                     XDG_SURFACE_GET_TOPLEVEL,
                                     &xdg_toplevel_interface, NULL);
    wl_proxy_add_listener((struct wl_proxy *)hws->toplevel,
                          (void (**)(void))&toplevel_listener, hws);

    xdg_toplevel_set_title(hws->toplevel, "Mansion Desktop");
    xdg_toplevel_set_app_id(hws->toplevel, "mansion-desktop");

    /* Commit the surface so the compositor schedules a configure. */
    fprintf(stderr, "[mansion] wl_surface_commit (size %dx%d)\n",
            display->window_width, display->window_height);
    wl_surface_commit(hws->surface);
    wl_display_roundtrip(wl_client);
    fprintf(stderr, "[mansion] roundtrip done, configured=%d, size=%dx%d\n",
            hws->configured, hws->width, hws->height);

    /* ─── Set up seat / keyboard / pointer input ──────────────────── */
    /*
     * On Wayland, the compositor sends seat.capabilities events when
     * input devices become available.  We must handle this event (not
     * leave it nullptr) or the compositor may not deliver pointer/
     * keyboard events to our window.
     */

    if (hws->seat) {
        /* Helper: set up keyboard, pointer, and relative pointer. */
        auto setup_input = [&](struct wl_seat *seat) {
            hws->keyboard = (struct wl_keyboard *)
                wl_seat_get_keyboard(seat);
            if (hws->keyboard) {
                wl_proxy_add_listener(
                    (struct wl_proxy *)hws->keyboard,
                    (void (**)(void))&keyboard_listener,
                    nullptr);
            }
            hws->pointer = (struct wl_pointer *)
                wl_seat_get_pointer(seat);
            if (hws->pointer) {
                wl_proxy_add_listener(
                    (struct wl_proxy *)hws->pointer,
                    (void (**)(void))&pointer_listener,
                    nullptr);
                /* Request relative pointer motion for smooth camera look. */
                if (hws->rel_ptr_mgr) {
                    hws->rel_ptr = (struct zwp_relative_pointer_v1 *)
                        zwp_relative_pointer_manager_v1_get_relative_pointer(
                            hws->rel_ptr_mgr, hws->pointer);
                    if (hws->rel_ptr) {
                        wl_proxy_add_listener(
                            (struct wl_proxy *)hws->rel_ptr,
                            (void (**)(void))&relative_pointer_listener,
                            nullptr);
                    }
                }
            }
        };

        /*
         * Initial setup (compositor likely already sent capabilities
         * during the roundtrip above).
         */
        setup_input(hws->seat);

        /*
         * Register the seat capabilities listener (defined at file
         * scope) so the compositor knows we handle input capability
         * changes.  This replaces the nullptr that caused KWin to
         * suppress pointer/keyboard events.
         */
        static constexpr struct wl_seat_listener seat_listener = {
            .capabilities = seat_capabilities,
            .name         = nullptr,
        };

        wl_proxy_add_listener((struct wl_proxy *)hws->seat,
                              (void (**)(void))&seat_listener,
                              hws);
    }

    /* The configure callback will fill in width/height.
     * If we haven't received one yet (shouldn't happen), use defaults. */
    if (!hws->configured) {
        hws->width  = display->window_width;
        hws->height = display->window_height;
    }

    /* Store in display struct. */
    display->wl_client_display = wl_client;
    display->wl_surface        = hws->surface;

    return true;
}

void destroy_wayland_client_window(struct MansionDisplay* display)
{
    if (!display) return;

    if (display->wl_surface) {
        wl_surface_destroy(display->wl_surface);
        display->wl_surface = nullptr;
    }
    if (display->wl_client_display) {
        wl_display_flush(display->wl_client_display);
        wl_display_disconnect(display->wl_client_display);
        display->wl_client_display = nullptr;
    }

    if (g_hws) {
        if (g_hws->wl_client) {
            wl_display_flush(g_hws->wl_client);
        }
        delete g_hws;
        g_hws = nullptr;
    }
}

/* ─── EGL setup for Wayland client display (surfaceless Mesa + PBuffer) ──────
 *
 * wl_egl_window + EGL_PLATFORM_WAYLAND_EXT is incompatible in Mesa —
 * eglCreateWindowSurface fails with EGL_BAD_NATIVE_WINDOW.
 *
 * Solution: use EGL_MESA_platform_surfaceless with a PBuffer surface for
 * GPU rendering, then read pixels via glReadPixels and export them via
 * wl_shm buffers to the host compositor.  This is the same approach the
 * test client uses and is known to work reliably.
 */

static bool create_egl_for_wayland(struct MansionDisplay* display)
{
    if (!display->wl_client_display) return false;

    /* Check for surfaceless Mesa support. */
    const char* ext_str = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    bool has_surfaceless = ext_str && strstr(ext_str, "EGL_MESA_platform_surfaceless");
    if (!has_surfaceless) {
        std::cerr << "Wayland EGL: EGL_MESA_platform_surfaceless not available"
                  << std::endl;
        return false;
    }

    EGLDisplay egl_dpy = eglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA,
                                                nullptr, nullptr);
    if (egl_dpy == EGL_NO_DISPLAY) {
        std::cerr << "Wayland EGL: failed to get surfaceless display" << std::endl;
        return false;
    }

    EGLint egl_major, egl_minor;
    if (!eglInitialize(egl_dpy, &egl_major, &egl_minor)) {
        std::cerr << "Wayland EGL: init failed, eglGetError="
                  << eglGetError() << std::endl;
        eglTerminate(egl_dpy);
        return false;
    }
    fprintf(stderr, "[mansion] EGL (surfaceless): v%d.%d\n", egl_major, egl_minor);

    /* PBuffer requires EGL_PBUFFER_BIT in the config. */
    EGLint pbuffer_config_attr[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 0,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    EGLConfig egl_config;
    EGLint egl_config_count;
    if (!eglChooseConfig(egl_dpy, pbuffer_config_attr, &egl_config, 1,
                         &egl_config_count) || egl_config_count == 0) {
        std::cerr << "Wayland EGL: choose config failed, eglGetError="
                  << eglGetError() << std::endl;
        eglTerminate(egl_dpy);
        return false;
    }

    EGLContext egl_ctx = eglCreateContext(egl_dpy, egl_config,
                                          EGL_NO_CONTEXT, context_attr);
    if (egl_ctx == EGL_NO_CONTEXT) {
        std::cerr << "Wayland EGL: context creation failed, eglGetError="
                  << eglGetError() << std::endl;
        eglTerminate(egl_dpy);
        return false;
    }

    /* Create a PBuffer surface with the window dimensions. */
    EGLint pbuffer_attr[] = {
        EGL_WIDTH, display->window_width,
        EGL_HEIGHT, display->window_height,
        EGL_NONE
    };

    EGLSurface egl_surf = eglCreatePbufferSurface(egl_dpy, egl_config, pbuffer_attr);
    if (egl_surf == EGL_NO_SURFACE) {
        std::cerr << "Wayland EGL: pbuffer creation failed, eglGetError="
                  << eglGetError() << std::endl;
        eglDestroyContext(egl_dpy, egl_ctx);
        eglTerminate(egl_dpy);
        return false;
    }
    fprintf(stderr, "[mansion] EGL: pbuffer surface created (%dx%d)\n",
            display->window_width, display->window_height);

    if (eglMakeCurrent(egl_dpy, egl_surf, egl_surf, egl_ctx) == EGL_FALSE) {
        std::cerr << "Wayland EGL: make current failed, eglGetError="
                  << eglGetError() << std::endl;
        eglDestroySurface(egl_dpy, egl_surf);
        eglDestroyContext(egl_dpy, egl_ctx);
        eglTerminate(egl_dpy);
        return false;
    }
    fprintf(stderr, "[mansion] EGL: make current OK\n");

    /* Enable vsync. */
    eglSwapInterval(egl_dpy, 1);

    display->egl_display = egl_dpy;
    display->egl_context = egl_ctx;
    display->egl_surface = egl_surf;
    display->egl_pbuffer_surface = egl_surf;
    display->egl_config  = egl_config;
    display->egl_config_count = egl_config_count;

    return true;
}

/* ─── create_egl_and_window (Wayland version) ─────────────────────────────── */

bool create_egl_and_window(struct MansionDisplay* mansion_display)
{
    /* Set up the Wayland client window (reconnect path). */
    if (!setup_wayland_client_window(mansion_display)) {
        return false;
    }

    if (!create_egl_for_wayland(mansion_display)) {
        destroy_wayland_client_window(mansion_display);
        return false;
    }

    /* Reinitialize the renderer (shaders, etc.) on the new EGL context. */
    if (!init_renderer(mansion_display)) {
        std::cerr << "Reconnect: Failed to reinitialize renderer" << std::endl;
        eglMakeCurrent(mansion_display->egl_display, EGL_NO_SURFACE,
                       EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroySurface(mansion_display->egl_display,
                          mansion_display->egl_surface);
        eglDestroyContext(mansion_display->egl_display,
                          mansion_display->egl_context);
        eglTerminate(mansion_display->egl_display);
        destroy_wayland_client_window(mansion_display);
        return false;
    }

    return true;
}

/* ─── create_display (Wayland client path) ─────────────────────────────────── */

struct MansionDisplay* create_display(struct MansionCompositor* compositor,
                                       struct wl_display* wl_display)
{
    auto* mansion_display = new MansionDisplay;
    mansion_display->compositor = compositor;
    mansion_display->wl_display = wl_display;
    mansion_display->egl_config_count = 0;
    mansion_display->window_width = 1024;
    mansion_display->window_height = 768;
    mansion_display->renderer = nullptr;
    mansion_display->egl_display = EGL_NO_DISPLAY;
    mansion_display->egl_context = EGL_NO_CONTEXT;
    mansion_display->egl_surface = EGLSurface(nullptr);
    mansion_display->wl_client_display = nullptr;
    mansion_display->wl_surface = nullptr;

    /* Create the host window using native Wayland client + EGL. */
    if (!create_egl_and_window(mansion_display)) {
        std::cerr << "Warning: Failed to create Wayland window + EGL, running headless"
                  << std::endl;
        delete mansion_display;
        return nullptr;
    }

    return mansion_display;
}

/* ─── create_display_headless (unchanged) ──────────────────────────────────── */

struct MansionDisplay* create_display_headless(struct MansionCompositor* compositor,
                                                struct wl_display* wl_display)
{
    auto* mansion_display = new MansionDisplay;
    mansion_display->compositor = compositor;
    mansion_display->wl_display = wl_display;
    mansion_display->egl_display = EGL_NO_DISPLAY;
    mansion_display->egl_context = EGL_NO_CONTEXT;
    mansion_display->egl_surface = EGL_NO_SURFACE;
    mansion_display->egl_config = nullptr;
    mansion_display->egl_config_count = 0;
    mansion_display->window_width = 1024;
    mansion_display->window_height = 768;
    mansion_display->renderer = nullptr;

    /* Try to set up EGL with surfaceless Mesa for offscreen rendering.
     * If EGL is unavailable, keep running without a renderer and still
     * fire frame callbacks. */
    EGLint egl_major, egl_minor;
    EGLint num_configs = 0;

    /* Determine EGL platform: use surfaceless Mesa if available. */
    const char* ext_str = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    bool has_surfaceless = ext_str && strstr(ext_str, "EGL_MESA_platform_surfaceless");

    EGLDisplay egl_dpy;
    if (has_surfaceless) {
        egl_dpy = eglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA, nullptr, nullptr);
        if (egl_dpy == EGL_NO_DISPLAY) {
            std::cerr << "Headless EGL: surfaceless Mesa not available" << std::endl;
            return mansion_display;
        }
    } else {
        egl_dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (egl_dpy == EGL_NO_DISPLAY) {
            std::cerr << "Headless EGL: cannot get display" << std::endl;
            return mansion_display;
        }
    }

    if (!eglInitialize(egl_dpy, &egl_major, &egl_minor)) {
        std::cerr << "Headless EGL: init failed" << std::endl;
        if (has_surfaceless) eglTerminate(egl_dpy);
        return mansion_display;
    }

    /* Surfaceless Mesa requires EGL_PBUFFER_BIT in the config attributes. */
    EGLint headless_config_attr[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 0,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    if (!eglChooseConfig(egl_dpy, headless_config_attr, &mansion_display->egl_config, 1, &num_configs) || num_configs == 0) {
        std::cerr << "Headless EGL: no config" << std::endl;
        eglTerminate(egl_dpy);
        return mansion_display;
    }

    mansion_display->egl_display = egl_dpy;

    mansion_display->egl_context = eglCreateContext(egl_dpy,
                                                     mansion_display->egl_config,
                                                     EGL_NO_CONTEXT, context_attr);
    if (mansion_display->egl_context == EGL_NO_CONTEXT) {
        std::cerr << "Headless EGL: context creation failed" << std::endl;
        eglTerminate(egl_dpy);
        return mansion_display;
    }

    EGLint pbuffer_attr[] = {
        EGL_WIDTH, 1024,
        EGL_HEIGHT, 768,
        EGL_NONE
    };

    EGLSurface egl_surf = eglCreatePbufferSurface(egl_dpy, mansion_display->egl_config, pbuffer_attr);
    if (egl_surf == EGL_NO_SURFACE) {
        std::cerr << "Headless EGL: pbuffer creation failed" << std::endl;
        eglDestroyContext(egl_dpy, mansion_display->egl_context);
        eglTerminate(egl_dpy);
        return mansion_display;
    }

    mansion_display->egl_surface = egl_surf;

    if (eglMakeCurrent(egl_dpy, egl_surf, egl_surf, mansion_display->egl_context) == EGL_FALSE) {
        std::cerr << "Headless EGL: make current failed" << std::endl;
        eglDestroySurface(egl_dpy, egl_surf);
        eglDestroyContext(egl_dpy, mansion_display->egl_context);
        eglTerminate(egl_dpy);
        return mansion_display;
    }

    /* Enable vsync to prevent buffer swap tearing. */
    eglSwapInterval(egl_dpy, 1);

    glViewport(0, 0, mansion_display->window_width, mansion_display->window_height);

    if (!init_renderer(mansion_display)) {
        std::cerr << "Headless EGL: renderer init failed" << std::endl;
        eglDestroySurface(egl_dpy, egl_surf);
        eglDestroyContext(egl_dpy, mansion_display->egl_context);
        eglTerminate(egl_dpy);
        return mansion_display;
    }

    return mansion_display;
}

/* ─── Resize (Wayland version) ─────────────────────────────────────────────── */

void display_resize(struct MansionDisplay* display) {
    if (!display || !display->renderer) return;
    glViewport(0, 0, display->window_width, display->window_height);
    if (display->renderer->viewport_uniform >= 0) {
        glUniform2f(display->renderer->viewport_uniform,
                    static_cast<GLfloat>(display->window_width),
                    static_cast<GLfloat>(display->window_height));
    }

    /* Send a new configure to the focused toplevel so it can resize. */
    if (display->compositor->focused_surface_resource) {
        auto* surface = compositor_surface_from_resource(display->compositor->focused_surface_resource);
        if (surface && surface->xdg_surface && xdg_surface_has_toplevel(surface->xdg_surface)) {
            xdg_shell_send_configure_resize(surface->xdg_surface,
                                            display->window_width, display->window_height);
        }
    }
}

/* ─── destroy_display (Wayland version) ────────────────────────────────────── */

void destroy_display(struct MansionDisplay* display) {
    if (!display) return;

    destroy_renderer(display);

    /* Clean up Wayland client window. */
    if (display->wl_client_display || display->wl_surface) {
        destroy_wayland_client_window(display);
    }

    if (display->egl_display != EGL_NO_DISPLAY) {
        eglMakeCurrent(display->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (display->egl_surface != EGL_NO_SURFACE) {
            eglDestroySurface(display->egl_display, display->egl_surface);
        }
        eglDestroyContext(display->egl_display, display->egl_context);
        eglTerminate(display->egl_display);
    }

    delete display;
}

struct MansionRenderer* get_renderer(struct MansionDisplay* display) {
    return display ? display->renderer : nullptr;
}

EGLContext get_egl_context(struct MansionDisplay* display) {
    return display ? display->egl_context : EGL_NO_CONTEXT;
}

bool init_renderer(struct MansionDisplay* display) {
    static const char* vertex_shader_source =
        "precision mediump float;\n"
        "uniform vec2 viewport;\n"
        "attribute vec2 pos;\n"
        "attribute vec2 texcoord;\n"
        "varying vec2 v_texcoord;\n"
        "void main() {\n"
        "    gl_Position = vec4((pos.x / viewport.x) * 2.0 - 1.0, "
        "                       1.0 - (pos.y / viewport.y) * 2.0, "
        "                       0.0, 1.0);\n"
        "    v_texcoord = texcoord;\n"
        "}\n";

    static const char* fragment_shader_source =
        "precision mediump float;\n"
        "uniform vec4 color;\n"
        "uniform sampler2D tex;\n"
        "varying vec2 v_texcoord;\n"
        "void main() {\n"
        "    if (int(color.a) == 0)\n"
        "        gl_FragColor = texture2D(tex, v_texcoord);\n"
        "    else\n"
        "        gl_FragColor = color;\n"
        "}\n";

    auto* renderer = new MansionRenderer();
    renderer->program = create_program(vertex_shader_source, fragment_shader_source);
    if (!renderer->program) {
        delete renderer;
        return false;
    }

    glUseProgram(renderer->program);
    renderer->pos_attrib = glGetAttribLocation(renderer->program, "pos");
    renderer->tex_attrib = glGetAttribLocation(renderer->program, "texcoord");
    renderer->color_uniform = glGetUniformLocation(renderer->program, "color");
    renderer->tex_uniform = glGetUniformLocation(renderer->program, "tex");
    renderer->viewport_uniform = glGetUniformLocation(renderer->program, "viewport");

    /* Set viewport uniform so pixel coordinates map to clip space. */
    glUniform2f(renderer->viewport_uniform,
                static_cast<GLfloat>(display->window_width),
                static_cast<GLfloat>(display->window_height));

    /* P2-T02: 3D panel shader with MVP uniform. */
    static const char* vs_3d =
        "precision mediump float;\n"
        "uniform mat4 mvp;\n"
        "attribute vec3 pos;\n"
        "attribute vec2 texcoord;\n"
        "varying vec2 v_texcoord;\n"
        "void main() {\n"
        "    gl_Position = mvp * vec4(pos, 1.0);\n"
        "    v_texcoord = texcoord;\n"
        "}\n";

    renderer->program_3d = create_program(vs_3d, fragment_shader_source);
    if (!renderer->program_3d) {
        /* Fall back to 2D-only if 3D shader fails. */
        glUseProgram(0);
        display->renderer = renderer;
        return true;
    }
    glUseProgram(renderer->program_3d);
    renderer->pos_3d = glGetAttribLocation(renderer->program_3d, "pos");
    renderer->tex_3d = glGetAttribLocation(renderer->program_3d, "texcoord");
    renderer->mvp_uniform = glGetUniformLocation(renderer->program_3d, "mvp");
    renderer->color_3d_uniform =
        glGetUniformLocation(renderer->program_3d, "color");
    renderer->tex_3d_uniform =
        glGetUniformLocation(renderer->program_3d, "tex");
    glUseProgram(0);

    glClearColor(0.15f, 0.15f, 0.2f, 1.0f);

    display->renderer = renderer;
    return true;
}

void destroy_renderer(struct MansionDisplay* display) {
    if (!display || !display->renderer) return;

    glUseProgram(0);
    glDeleteProgram(display->renderer->program);
    if (display->renderer->program_3d)
        glDeleteProgram(display->renderer->program_3d);
    delete display->renderer;
    display->renderer = nullptr;
}

void render_surface(struct MansionDisplay* display, struct wl_resource* surface, int32_t x, int32_t y);
void render_surface_from_data(struct MansionDisplay* display, struct MansionSurface* surface_data,
                              int32_t x, int32_t y);

static void fire_frame_callbacks(struct MansionCompositor* compositor) {
    static uint32_t tick = 0;
    ++tick;
    struct MansionSurface *surface, *next;
    wl_list_for_each_safe(surface, next, &compositor->surface_list, link) {
        struct wl_resource *cb, *cb_next;
        wl_list_for_each_safe(cb, cb_next, &surface->frame_callback_list, link) {
            wl_list_remove(wl_resource_get_link(cb));
            wl_callback_send_done(cb, tick);
            wl_resource_destroy(cb);
        }
    }
}

// ── P2-T02: 3D panel rendering ─────────────────────────────────────────────

static void render_panel(struct MansionDisplay* display) {
    auto* renderer = display->renderer;
    auto* compositor = display->compositor;
    if (!renderer || !compositor || !renderer->program_3d) return;

    /* Pick the surface to project onto the panel.
     * Prefer the focused surface; fall back to any surface with a
     * buffer or texture in surface_list, then orphaned_surfaces
     * (client may have disconnected but surface still valid). */
    struct MansionSurface* target = nullptr;
    if (compositor->focused_surface_resource) {
        target = compositor_surface_from_resource(
            compositor->focused_surface_resource);
    }
    if (!target) {
        struct MansionSurface *s;
        wl_list_for_each(s, &compositor->surface_list, link) {
            if (s->gl_texture || s->buffer_resource) {
                target = s;
                break;
            }
        }
    }
    if (!target) {
        struct MansionSurface *s;
        wl_list_for_each_reverse(s, &compositor->orphaned_surfaces, link) {
            if (s->gl_texture || s->buffer_resource) {
                target = s;
                break;
            }
        }
    }
    if (!target) return;   /* Nothing to draw */

    /* Release the buffer after rendering (simple headless semantics). */
    bool released = false;
    auto release_buffer = [&]() {
        if (released || !target->buffer_resource) return;
        wl_buffer_send_release(target->buffer_resource);
        target->buffer_resource = nullptr;
        released = true;
    };

    /* Ensure the surface has a texture (re-upload only when buffer changed). */
    if (target->buffer_resource && target->needs_upload) {
        struct wl_shm_buffer* shm_buf = wl_shm_buffer_get(
            target->buffer_resource);
        if (shm_buf) {
            wl_shm_buffer_begin_access(shm_buf);
            void* data = wl_shm_buffer_get_data(shm_buf);
            if (data) {
                int w = wl_shm_buffer_get_width(shm_buf);
                int h = wl_shm_buffer_get_height(shm_buf);
                GLuint tex = 0;
                glGenTextures(1, &tex);
                glBindTexture(GL_TEXTURE_2D, tex);
                /* Format-aware texture upload: query the wl_shm buffer format
                 * and swizzle on the CPU to GL_RGBA so results are correct
                 * regardless of Mesa driver internals.  */
                int format = wl_shm_buffer_get_format(shm_buf);
                if (format == WL_SHM_FORMAT_ABGR8888 ||
                    format == WL_SHM_FORMAT_XBGR8888) {
                    /* ABGR8888 on little-endian: bytes are [R, G, B, A].
                     * GL_RGBA expects [R, G, B, A], so no swizzle needed. */
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
                                 GL_RGBA, GL_UNSIGNED_BYTE, data);
                } else {
                    /* Default: ARGB8888 / XRGB8888 on little-endian have
                     * bytes [B, G, R, A].  Swizzle to GL_RGBA [R, G, B, A]. */
                    std::vector<uint8_t> rgba(w * h * 4);
                    for (int y = 0; y < h; ++y) {
                        for (int x = 0; x < w; ++x) {
                            int idx = (y * w + x) * 4;
                            rgba[idx + 0] =
                                ((const uint8_t*)data)[idx + 2];  // R
                            rgba[idx + 1] =
                                ((const uint8_t*)data)[idx + 1];  // G
                            rgba[idx + 2] =
                                ((const uint8_t*)data)[idx + 0];  // B
                            rgba[idx + 3] =
                                ((const uint8_t*)data)[idx + 3];  // A
                        }
                    }
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
                                 GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
                }
                renderer->bytes_uploaded +=
                    static_cast<long long>(w) * h * 4;
                glTexParameteri(GL_TEXTURE_2D,
                                GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D,
                                GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                if (target->gl_texture)
                    glDeleteTextures(1, &target->gl_texture);
                target->gl_texture = tex;
            }
            wl_shm_buffer_end_access(shm_buf);
            target->needs_upload = false;
        }
    }

    if (!target->gl_texture) {
        release_buffer();
        return;
    }

    /* ── Build MVP matrix ─────────────────────────────────────────── */
    math::vec3 cam_pos(display->camera.x, display->camera.y,
                       display->camera.z);
    math::vec3 panel_c(display->panel.x, display->panel.y,
                       display->panel.z);

    math::mat4 view   = math::look_at(cam_pos, panel_c,
                                      math::vec3{0, 1, 0});
    math::mat4 model  = math::translate(display->panel.x,
                                        display->panel.y,
                                        display->panel.z);
    float aspect =
        static_cast<float>(display->window_width) /
        static_cast<float>(display->window_height);
    math::mat4 proj =
        math::perspective(display->camera.fov, aspect, 0.1f, 100.0f);

    math::mat4 mvp = math::mat4::multiply(proj,
                                          math::mat4::multiply(view, model));

    /* ── Panel quad (triangle strip) ──────────────────────────────── */
    float pw = display->panel.width;
    float ph = display->panel.height;
    GLfloat verts[] = {
        /*   x       y       z      u  v */
        -pw * 0.5f, -ph * 0.5f, 0.0f,  0.0f, 0.0f,
         pw * 0.5f, -ph * 0.5f, 0.0f,  1.0f, 0.0f,
        -pw * 0.5f,  ph * 0.5f, 0.0f,  0.0f, 1.0f,
         pw * 0.5f,  ph * 0.5f, 0.0f,  1.0f, 1.0f,
    };

    glUseProgram(renderer->program_3d);

    glUniformMatrix4fv(renderer->mvp_uniform, 1, GL_FALSE, mvp.m);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, target->gl_texture);
    glUniform1i(renderer->tex_3d_uniform, 0);
    /* P3-T05: green tint when panel is targeted. */
    if (display->panel_targeted) {
        glUniform4f(renderer->color_3d_uniform, 0.0f, 1.0f, 0.0f, 0.0f);
    } else {
        glUniform4f(renderer->color_3d_uniform, 1.0f, 1.0f, 1.0f, 0.0f);
    }

    glEnableVertexAttribArray(renderer->pos_3d);
    glVertexAttribPointer(renderer->pos_3d, 3, GL_FLOAT, GL_FALSE,
                          5 * sizeof(float), &verts[0]);
    glEnableVertexAttribArray(renderer->tex_3d);
    glVertexAttribPointer(renderer->tex_3d, 2, GL_FLOAT, GL_FALSE,
                          5 * sizeof(float), &verts[3]);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(renderer->pos_3d);
    glDisableVertexAttribArray(renderer->tex_3d);

    glUseProgram(0);

    /* Release the buffer after rendering. */
    release_buffer();
}

void render(struct MansionDisplay* display) {
    if (!display) return;

    /* P2-T07: track frame timing when --stats is enabled. */
    fprintf(stderr, "[mansion] render frame (size %dx%d, flat=%d, room=%d)\n",
            display->window_width, display->window_height,
            display->flat_mode, display->room_mode);
    auto frame_start = std::chrono::steady_clock::now();

    // Fire frame callbacks on every render tick (needed even in headless mode).
    fire_frame_callbacks(display->compositor);

    auto* renderer = display->renderer;
    if (!renderer) return;

    glClear(GL_COLOR_BUFFER_BIT);

    if (display->flat_mode) {
        // P2-T02: 2D rendering path (backwards compatible).
        struct MansionSurface *surface;
        wl_list_for_each_reverse(surface, &display->compositor->surface_list, link) {
            render_surface(display, surface->resource, 0, 0);
        }
        {
            /* Render orphaned surfaces oldest-first so the newest
             * (last-connected) surface is drawn last and sits on top. */
            struct MansionSurface *surface;
            wl_list_for_each(surface, &display->compositor->orphaned_surfaces, link) {
                render_surface_from_data(display, surface, 0, 0);
            }
        }
    } else if (input_mode_get() == InputMode::Application) {
        // P3-T04: Application mode — render focused surface fullscreen (2D).
        render_application_fullscreen(display);
    } else {
        /* P4-T01: World mode — optionally draw room geometry as background,
         * then the 3D panel. */
        if (display->room_mode) {
            auto* renderer = display->renderer;
            glUseProgram(renderer->program_3d);

            /* Compute MVP: room is in world space, so model = identity. */
            math::vec3 cam_pos(display->camera.x, display->camera.y,
                               display->camera.z);
            float yaw = display->camera.yaw;
            float pitch = display->camera.pitch;
            /* Forward direction (same convention as apply_movement in input.cpp). */
            float fwdX = -std::sin(yaw) * std::cos(pitch);
            float fwdY =  std::sin(pitch);
            float fwdZ = -std::cos(yaw) * std::cos(pitch);
            math::vec3 cam_target(cam_pos.x + fwdX * 10.0f,
                                  cam_pos.y + fwdY * 10.0f,
                                  cam_pos.z + fwdZ * 10.0f);
            math::mat4 view = math::look_at(cam_pos, cam_target,
                                            math::vec3{0, 1, 0});

            float aspect =
                static_cast<float>(display->window_width) /
                static_cast<float>(display->window_height);
            math::mat4 proj =
                math::perspective(display->camera.fov, aspect, 0.1f, 100.0f);
            math::mat4 mvp = math::mat4::multiply(proj, view);
            glUniformMatrix4fv(renderer->mvp_uniform, 1, GL_FALSE, mvp.m);

            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LEQUAL);
            glBindTexture(GL_TEXTURE_2D, 0);
            room_geometry_init(nullptr);
            {
                const RoomGeometry* geo = room_geometry_get();
                for (size_t i = 0; i < geo->mesh_count; i++) {
                    const RoomMesh& mesh = geo->meshes[i];
                    if (!mesh.vertices || mesh.vertex_count == 0) continue;
                    glEnableVertexAttribArray(renderer->pos_3d);
                    glVertexAttribPointer(
                        renderer->pos_3d,
                        3,
                        GL_FLOAT,
                        GL_FALSE,
                        5 * sizeof(float),
                        mesh.vertices);
                    glEnableVertexAttribArray(renderer->tex_3d);
                    glVertexAttribPointer(
                        renderer->tex_3d,
                        2,
                        GL_FLOAT,
                        GL_FALSE,
                        5 * sizeof(float),
                        &mesh.vertices[3]);
                    glUniform4fv(renderer->color_3d_uniform, 1, mesh.color);
                    glDrawArrays(
                        GL_TRIANGLE_STRIP,
                        0,
                        static_cast<GLsizei>(mesh.vertex_count));
                    glDisableVertexAttribArray(renderer->pos_3d);
                    glDisableVertexAttribArray(renderer->tex_3d);
                }
            }
            glDisable(GL_DEPTH_TEST);
        }
        // P2-T02: 3D panel rendering — draw focused surface on a perspective panel.
        render_panel(display);
    }

    swap_buffers(display);

    /* P2-T07: accumulate stats and print every 60 frames. */
    auto frame_end = std::chrono::steady_clock::now();
    if (display->stats_enabled && renderer) {
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(frame_end - frame_start).count();
        display->stats_total_render_us += us;
        display->stats_total_bytes_up += renderer->bytes_uploaded;
        display->stats_frame_count++;
        if (display->stats_frame_count >= 60) {
            double avg_us = static_cast<double>(display->stats_total_render_us) / display->stats_frame_count;
            double avg_fps = 1'000'000.0 / avg_us;
            std::cerr << "stats: " << display->stats_frame_count << " frames, "
                      << avg_fps << " fps, "
                      << display->stats_total_bytes_up / 1024.0 << " KiB uploaded" << std::endl;
            display->stats_total_render_us = 0;
            display->stats_total_bytes_up = 0;
            display->stats_frame_count = 0;
        }
    }

    /* P2-T04: Check for GL errors after each frame in debug builds.
     * Log each unique error once per frame so we don't spam the log. */
#ifndef NDEBUG
    {
        static GLuint seen_errors = 0;
        GLuint err;
        while ((err = glGetError()) != GL_NO_ERROR) {
            if (!(seen_errors & (1u << (err & 0x1f)))) {
                seen_errors |= (1u << (err & 0x1f));
                const char* err_name = "UNKNOWN";
                switch (err) {
                    case GL_INVALID_ENUM:       err_name = "GL_INVALID_ENUM"; break;
                    case GL_INVALID_VALUE:      err_name = "GL_INVALID_VALUE"; break;
                    case GL_INVALID_OPERATION:  err_name = "GL_INVALID_OPERATION"; break;
                    case GL_INVALID_FRAMEBUFFER_OPERATION:
                        err_name = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
                    case GL_OUT_OF_MEMORY:      err_name = "GL_OUT_OF_MEMORY"; break;
                }
                std::cerr << "GL error: " << err_name << " (0x"
                          << std::hex << err << std::dec << ")" << std::endl;
            }
        }
    }
#endif
}

void render_surface(struct MansionDisplay* display, struct wl_resource* surface, int32_t x, int32_t y) {
    auto* surface_data = compositor_surface_from_resource(surface);

    if (!surface_data) {
        return;
    }

    int32_t sx = static_cast<int32_t>(x);
    int32_t sy = static_cast<int32_t>(y);
    if (!surface_data->has_current_position) {
        sx = 0;
        sy = 0;
    } else {
        sx += surface_data->current_x;
        sy += surface_data->current_y;
    }

    auto* renderer = display->renderer;
    if (!renderer) return;

    /* Determine surface size. */
    float sw = (surface_data->width > 0) ? static_cast<float>(surface_data->width) : 200.0f;
    float sh = (surface_data->height > 0) ? static_cast<float>(surface_data->height) : 150.0f;

    glUseProgram(renderer->program);
    if (renderer->viewport_uniform >= 0) {
        glUniform2f(renderer->viewport_uniform,
                    static_cast<GLfloat>(display->window_width),
                    static_cast<GLfloat>(display->window_height));
    }

    if (surface_data->buffer_resource) {
        struct wl_shm_buffer* shm_buf = wl_shm_buffer_get(surface_data->buffer_resource);
        if (shm_buf) {
            wl_shm_buffer_begin_access(shm_buf);
            void* data = wl_shm_buffer_get_data(shm_buf);
            if (data) {
                int w = wl_shm_buffer_get_width(shm_buf);
                int h = wl_shm_buffer_get_height(shm_buf);
                GLuint tex = 0;
                glGenTextures(1, &tex);
                glBindTexture(GL_TEXTURE_2D, tex);

                /* Format-aware texture upload: query the wl_shm buffer format
                 * and swizzle on the CPU to GL_RGBA so results are correct
                 * regardless of Mesa driver internals.  */
                int format = wl_shm_buffer_get_format(shm_buf);
                if (format == WL_SHM_FORMAT_ABGR8888 ||
                    format == WL_SHM_FORMAT_XBGR8888) {
                    /* ABGR8888 on little-endian: bytes are [R, G, B, A].
                     * GL_RGBA expects [R, G, B, A], so no swizzle needed. */
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
                                 GL_UNSIGNED_BYTE, data);
                } else {
                    /* Default: ARGB8888 / XRGB8888 on little-endian have
                     * bytes [B, G, R, A].  Swizzle to GL_RGBA [R, G, B, A]. */
                    std::vector<uint8_t> rgba(w * h * 4);
                    for (int y = 0; y < h; y++) {
                        for (int x = 0; x < w; x++) {
                            int idx = (y * w + x) * 4;
                            rgba[idx + 0] = ((const uint8_t*)data)[idx + 2];  // R
                            rgba[idx + 1] = ((const uint8_t*)data)[idx + 1];  // G
                            rgba[idx + 2] = ((const uint8_t*)data)[idx + 0];  // B
                            rgba[idx + 3] = ((const uint8_t*)data)[idx + 3];  // A
                        }
                    }
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
                                 GL_UNSIGNED_BYTE, rgba.data());
                }
                renderer->bytes_uploaded +=
                    static_cast<long long>(w) * h * 4;
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                surface_data->gl_texture = tex;
            }
            wl_shm_buffer_end_access(shm_buf);
        }
    }

    if (surface_data->gl_texture) {
        /* Render textured surface. */
        GLfloat attrib_data[] = {
            static_cast<GLfloat>(sx),      static_cast<GLfloat>(sy),      0.0f, 0.0f,
            static_cast<GLfloat>(sx) + sw, static_cast<GLfloat>(sy),      1.0f, 0.0f,
            static_cast<GLfloat>(sx),      static_cast<GLfloat>(sy) + sh, 0.0f, 1.0f,
            static_cast<GLfloat>(sx) + sw, static_cast<GLfloat>(sy) + sh, 1.0f, 1.0f,
        };

        glEnableVertexAttribArray(renderer->pos_attrib);
        glVertexAttribPointer(renderer->pos_attrib, 2, GL_FLOAT, GL_FALSE,
                              4 * sizeof(float), &attrib_data[0]);

        glEnableVertexAttribArray(renderer->tex_attrib);
        glVertexAttribPointer(renderer->tex_attrib, 2, GL_FLOAT, GL_FALSE,
                              4 * sizeof(float), &attrib_data[2]);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, surface_data->gl_texture);
        glUniform1i(renderer->tex_uniform, 0);
        glUniform4f(renderer->color_uniform, 1.0f, 1.0f, 1.0f, 0.0f);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glDisableVertexAttribArray(renderer->pos_attrib);
        glDisableVertexAttribArray(renderer->tex_attrib);
    } else {
        /* Render solid color placeholder (no buffer yet). */
        GLfloat attrib_data[] = {
            static_cast<GLfloat>(sx),      static_cast<GLfloat>(sy),
            static_cast<GLfloat>(sx) + sw, static_cast<GLfloat>(sy),
            static_cast<GLfloat>(sx),      static_cast<GLfloat>(sy) + sh,
            static_cast<GLfloat>(sx) + sw, static_cast<GLfloat>(sy) + sh,
        };

        glEnableVertexAttribArray(renderer->pos_attrib);
        glVertexAttribPointer(renderer->pos_attrib, 2, GL_FLOAT, GL_FALSE,
                              2 * sizeof(float), attrib_data);
        glDisableVertexAttribArray(renderer->tex_attrib);

        glUniform4f(renderer->color_uniform, 0.3f, 0.3f, 0.4f, 1.0f);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glDisableVertexAttribArray(renderer->pos_attrib);
    }

    /* Release the buffer now that it has been rendered (simple headless semantics). */
    if (surface_data->buffer_resource) {
        wl_buffer_send_release(surface_data->buffer_resource);
        surface_data->buffer_resource = nullptr;
    }
}

/* Render the focused surface fullscreen (P3-T04). */
static void render_application_fullscreen(struct MansionDisplay* display) {
    auto* renderer = display->renderer;
    if (!renderer) return;

    auto* comp = display->compositor;
    if (!comp || !comp->focused_surface_resource) return;

    auto* surface_data = compositor_surface_from_resource(comp->focused_surface_resource);
    if (!surface_data) return;

    /* Ensure the surface has a texture (re-upload when buffer changed). */
    if (surface_data->buffer_resource && surface_data->needs_upload) {
        struct wl_shm_buffer* shm_buf = wl_shm_buffer_get(surface_data->buffer_resource);
        if (shm_buf) {
            wl_shm_buffer_begin_access(shm_buf);
            void* data = wl_shm_buffer_get_data(shm_buf);
            if (data) {
                int w = wl_shm_buffer_get_width(shm_buf);
                int h = wl_shm_buffer_get_height(shm_buf);
                GLuint tex = 0;
                glGenTextures(1, &tex);
                glBindTexture(GL_TEXTURE_2D, tex);
                /* Format-aware texture upload: query the wl_shm buffer format
                 * and swizzle on the CPU to GL_RGBA so results are correct
                 * regardless of Mesa driver internals.  */
                int format = wl_shm_buffer_get_format(shm_buf);
                if (format == WL_SHM_FORMAT_ABGR8888 ||
                    format == WL_SHM_FORMAT_XBGR8888) {
                    /* ABGR8888 on little-endian: bytes are [R, G, B, A].
                     * GL_RGBA expects [R, G, B, A], so no swizzle needed. */
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
                                 GL_RGBA, GL_UNSIGNED_BYTE, data);
                } else {
                    /* Default: ARGB8888 / XRGB8888 on little-endian have
                     * bytes [B, G, R, A].  Swizzle to GL_RGBA [R, G, B, A]. */
                    std::vector<uint8_t> rgba(w * h * 4);
                    for (int y = 0; y < h; ++y) {
                        for (int x = 0; x < w; ++x) {
                            int idx = (y * w + x) * 4;
                            rgba[idx + 0] =
                                ((const uint8_t*)data)[idx + 2];  // R
                            rgba[idx + 1] =
                                ((const uint8_t*)data)[idx + 1];  // G
                            rgba[idx + 2] =
                                ((const uint8_t*)data)[idx + 0];  // B
                            rgba[idx + 3] =
                                ((const uint8_t*)data)[idx + 3];  // A
                        }
                    }
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
                                 GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
                }
                renderer->bytes_uploaded +=
                    static_cast<long long>(w) * h * 4;
                glTexParameteri(GL_TEXTURE_2D,
                                GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D,
                                GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                if (surface_data->gl_texture)
                    glDeleteTextures(1, &surface_data->gl_texture);
                surface_data->gl_texture = tex;
            }
            wl_shm_buffer_end_access(shm_buf);
            surface_data->needs_upload = false;
        }
    }

    if (!surface_data->gl_texture) return;

    glUseProgram(renderer->program);
    if (renderer->viewport_uniform >= 0) {
        glUniform2f(renderer->viewport_uniform,
                    static_cast<GLfloat>(display->window_width),
                    static_cast<GLfloat>(display->window_height));
    }

    /* Full-screen quad covering the viewport. */
    GLfloat attrib_data[] = {
        0.0f, static_cast<GLfloat>(display->window_height),  0.0f, 1.0f,
        static_cast<GLfloat>(display->window_width), static_cast<GLfloat>(display->window_height), 1.0f, 1.0f,
        0.0f, 0.0f,                                      0.0f, 0.0f,
        static_cast<GLfloat>(display->window_width), 0.0f,  1.0f, 0.0f,
    };

    glEnableVertexAttribArray(renderer->pos_attrib);
    glVertexAttribPointer(renderer->pos_attrib, 2, GL_FLOAT, GL_FALSE,
                          4 * sizeof(float), &attrib_data[0]);

    glEnableVertexAttribArray(renderer->tex_attrib);
    glVertexAttribPointer(renderer->tex_attrib, 2, GL_FLOAT, GL_FALSE,
                          4 * sizeof(float), &attrib_data[2]);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, surface_data->gl_texture);
    glUniform1i(renderer->tex_uniform, 0);
    /* color.a == 0 → shader samples texture; a != 0 → outputs color directly. */
    glUniform4f(renderer->color_uniform, 1.0f, 1.0f, 1.0f, 0.0f);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(renderer->pos_attrib);
    glDisableVertexAttribArray(renderer->tex_attrib);

    /* Release the buffer after rendering. */
    if (surface_data->buffer_resource) {
        wl_buffer_send_release(surface_data->buffer_resource);
        surface_data->buffer_resource = nullptr;
    }
}

void render_surface_from_data(struct MansionDisplay* display, struct MansionSurface* surface_data,
                              int32_t x, int32_t y) {
    if (!surface_data) return;

    auto* renderer = display->renderer;
    if (!renderer) return;

    int32_t sx = static_cast<int32_t>(x);
    int32_t sy = static_cast<int32_t>(y);
    if (!surface_data->has_current_position) {
        sx = 0;
        sy = 0;
    } else {
        sx += surface_data->current_x;
        sy += surface_data->current_y;
    }

    /* Determine surface size. */
    float sw = (surface_data->width > 0) ? static_cast<float>(surface_data->width) : 200.0f;
    float sh = (surface_data->height > 0) ? static_cast<float>(surface_data->height) : 150.0f;

    glUseProgram(renderer->program);
    if (renderer->viewport_uniform >= 0) {
        glUniform2f(renderer->viewport_uniform,
                    static_cast<GLfloat>(display->window_width),
                    static_cast<GLfloat>(display->window_height));
    }

    /* Orphaned surfaces have no buffer_resource, but may still have gl_texture
       from a previous render. Draw with it. */
    if (surface_data->gl_texture) {
        GLfloat attrib_data[] = {
            static_cast<GLfloat>(sx),      static_cast<GLfloat>(sy),      0.0f, 0.0f,
            static_cast<GLfloat>(sx) + sw, static_cast<GLfloat>(sy),      1.0f, 0.0f,
            static_cast<GLfloat>(sx),      static_cast<GLfloat>(sy) + sh, 0.0f, 1.0f,
            static_cast<GLfloat>(sx) + sw, static_cast<GLfloat>(sy) + sh, 1.0f, 1.0f,
        };

        glEnableVertexAttribArray(renderer->pos_attrib);
        glVertexAttribPointer(renderer->pos_attrib, 2, GL_FLOAT, GL_FALSE,
                              4 * sizeof(float), &attrib_data[0]);

        glEnableVertexAttribArray(renderer->tex_attrib);
        glVertexAttribPointer(renderer->tex_attrib, 2, GL_FLOAT, GL_FALSE,
                              4 * sizeof(float), &attrib_data[2]);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, surface_data->gl_texture);
        glUniform1i(renderer->tex_uniform, 0);
        glUniform4f(renderer->color_uniform, 1.0f, 1.0f, 1.0f, 0.0f);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glDisableVertexAttribArray(renderer->pos_attrib);
        glDisableVertexAttribArray(renderer->tex_attrib);
    }
}

/* ─── wl_shm buffer creation helper ────────────────────────────────────────── */

static struct wl_buffer* create_shm_buffer(struct MansionDisplay* display,
                                            struct wl_shm* shm)
{
    if (!shm) return nullptr;

    int w = display->window_width;
    int h = display->window_height;
    int32_t stride = w * 4;  /* ARGB8888 = 4 bytes per pixel */
    int32_t size = stride * h;

    /* Create an anonymous memory file. */
    int fd = memfd_create("mansion-shm", 0);
    if (fd < 0) {
        std::cerr << "shm: memfd_create failed" << std::endl;
        return nullptr;
    }
    if (ftruncate(fd, size) < 0) {
        std::cerr << "shm: ftruncate failed" << std::endl;
        close(fd);
        return nullptr;
    }

    void* ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        std::cerr << "shm: mmap failed" << std::endl;
        close(fd);
        return nullptr;
    }

    /* Make sure the EGL context is current so glReadPixels works. */
    EGLDisplay egl_dpy = display->egl_display;
    EGLSurface egl_surf = display->egl_surface;
    EGLContext egl_ctx = display->egl_context;
    if (eglMakeCurrent(egl_dpy, egl_surf, egl_surf, egl_ctx) == EGL_FALSE) {
        std::cerr << "shm: eglMakeCurrent failed" << std::endl;
        munmap(ptr, size);
        close(fd);
        return nullptr;
    }

    /* Read pixels from the PBuffer surface.
     * glReadPixels reads GL_RGBA; we convert to ARGB8888 for the display. */
    std::vector<uint8_t> rgba(w * h * 4);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());

    /* Convert GL_RGBA [R,G,B,A] to ARGB8888 [A,R,G,B] on little-endian. */
    uint32_t* pixels = (uint32_t*)ptr;
    for (int i = 0; i < w * h; i++) {
        pixels[i] = (rgba[i * 4 + 3] & 0xff)                   /* A */
                    | ((rgba[i * 4 + 0] & 0xff) << 16)          /* R */
                    | ((rgba[i * 4 + 1] & 0xff) << 8)           /* G */
                    | ((rgba[i * 4 + 2] & 0xff));               /* B */
    }

    munmap(ptr, size);

    /* Create wl_shm buffer. */
    struct wl_shm_pool* pool = wl_shm_create_pool(shm, fd, size);
    struct wl_buffer* buffer = wl_shm_pool_create_buffer(
        pool, 0, w, h, stride, WL_SHM_FORMAT_ARGB8888);
    wl_shm_pool_destroy(pool);
    close(fd);

    return buffer;
}

void swap_buffers(struct MansionDisplay* display) {
    if (!display || !display->wl_surface) return;

    /* Get wl_shm from the host_window_state. */
    struct host_window_state* hws = g_hws;
    struct wl_shm* shm = hws ? hws->shm : nullptr;

    if (shm) {
        /* Create a wl_shm buffer with the rendered pixels. */
        struct wl_buffer* buf = create_shm_buffer(display, shm);
        if (buf) {
            /* Attach, damage, and commit. */
            wl_surface_attach(display->wl_surface, buf, 0, 0);
            wl_surface_damage(display->wl_surface, 0, 0,
                              display->window_width, display->window_height);
            wl_surface_commit(display->wl_surface);
            /* Destroy buffer immediately — compositor will release it. */
            wl_buffer_destroy(buf);
        }
    }

    /* Flush the Wayland client display. */
    if (display->wl_client_display) {
        wl_display_flush(display->wl_client_display);
    }
}

bool take_screenshot(struct MansionDisplay* display, const char* path) {
    if (!display || !display->renderer) {
        return false;
    }

    /* Make sure the EGL context is current. */
    if (eglMakeCurrent(display->egl_display, display->egl_surface,
                       display->egl_surface, display->egl_context) == EGL_FALSE) {
        std::cerr << "Screenshot: eglMakeCurrent failed" << std::endl;
        return false;
    }

    /* Render one final frame to capture the latest state. */
    render(display);

    int w = display->window_width;
    int h = display->window_height;
    std::vector<uint8_t> pixels(w * h * 4);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    /* Write PPM (binary P6). Flip Y so (0,0) is top-left. */
    FILE* fp = fopen(path, "wb");
    if (!fp) {
        std::cerr << "Screenshot: cannot open " << path << ": " << strerror(errno) << std::endl;
        return false;
    }

    fprintf(fp, "P6\n%d %d\n255\n", w, h);
    for (int y = h - 1; y >= 0; --y) {
        for (int x = 0; x < w; ++x) {
            int idx = (y * w + x) * 4;
            fputc(pixels[idx + 0], fp);  // R
            fputc(pixels[idx + 1], fp);  // G
            fputc(pixels[idx + 2], fp);  // B
        }
    }

    fclose(fp);
    std::cerr << "Screenshot saved: " << path << std::endl;
    return true;
}

/* P4-T01: log camera position to stderr (for test verification). */
void log_camera_position(struct MansionDisplay* display) {
    if (!display) return;
    auto* cam = &display->camera;
    std::cerr << "camera_pos " << cam->x << " " << cam->y << " " << cam->z << std::endl;
}

/* P4-T02: teleport camera to stored viewpoint facing the monitor. */
void input_teleport(struct MansionDisplay* display) {
    if (!display) return;
    display->camera.x    = display->teleport_x;
    display->camera.y    = display->teleport_y;
    display->camera.z    = display->teleport_z;
    display->camera.yaw  = display->teleport_yaw;
    display->camera.pitch = display->teleport_pitch;
}
