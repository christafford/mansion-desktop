#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <wayland-server-protocol.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "display.h"
#include "compositor.h"
#include "compositor-private.h"

struct MansionRenderer {
    GLuint program;
    GLint pos_attrib;
    GLint tex_attrib;
    GLint color_uniform;
    GLint tex_uniform;
    GLint viewport_uniform;
};

// Forward declarations
static GLuint create_shader(GLenum type, const char* source);
static GLuint create_program(const char* vertex_shader_source, const char* fragment_shader_source);

static const EGLint context_attr[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
};

static const EGLint config_attr[] = {
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_ALPHA_SIZE, 8,
    EGL_DEPTH_SIZE, 0,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_NONE
};

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

static GLuint create_program(const char* vertex_shader_source, const char* fragment_shader_source) {
    GLuint vertex_shader = create_shader(GL_VERTEX_SHADER, vertex_shader_source);
    if (!vertex_shader) return 0;

    GLuint fragment_shader = create_shader(GL_FRAGMENT_SHADER, fragment_shader_source);
    if (!fragment_shader) {
        glDeleteShader(vertex_shader);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        GLint length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        std::string log(length + 1, '\0');
        glGetProgramInfoLog(program, length, nullptr, &log[0]);
        std::cerr << "Program linkage failed: " << log << std::endl;
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        glDeleteProgram(program);
        return 0;
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    return program;
}

struct MansionDisplay* create_display(struct MansionCompositor* compositor, struct wl_display* wl_display) {
    auto* mansion_display = new MansionDisplay;
    mansion_display->compositor = compositor;
    mansion_display->wl_display = wl_display;
    mansion_display->egl_config_count = 0;
    mansion_display->window_width = 1024;
    mansion_display->window_height = 768;
    mansion_display->renderer = nullptr;
    mansion_display->egl_display = EGL_NO_DISPLAY;
    mansion_display->egl_context = EGL_NO_CONTEXT;
    mansion_display->egl_surface = EGL_NO_SURFACE;

    // Create X11 window for the host window
    Display* x_display = XOpenDisplay(nullptr);
    if (!x_display) {
        std::cerr << "Failed to open X11 display" << std::endl;
        delete mansion_display;
        return nullptr;
    }

    int screen = DefaultScreen(x_display);
    Window x_window = XCreateSimpleWindow(x_display, DefaultRootWindow(x_display),
                                          0, 0, mansion_display->window_width,
                                          mansion_display->window_height, 0,
                                          BlackPixel(x_display, screen),
                                          WhitePixel(x_display, screen));

    XMapWindow(x_display, x_window);
    XFlush(x_display);

    // Store X11 display and window directly in MansionDisplay.
    mansion_display->x_display = x_display;
    mansion_display->x_window  = x_window;
    XSetWindowBorderWidth(x_display, x_window, 2);
    Colormap border_color = XCreateColormap(x_display, DefaultRootWindow(x_display),
                                            DefaultVisual(x_display, screen), AllocNone);
    XSetWindowBorder(x_display, x_window, border_color);
    XFlush(x_display);

    // Select input events we care about.
    XSelectInput(x_display, x_window,
        KeyPressMask | KeyReleaseMask |
        ButtonPressMask | ButtonReleaseMask |
        PointerMotionMask |
        FocusChangeMask | StructureNotifyMask);
    XFlush(x_display);

    // Create EGL display for X11
    mansion_display->egl_display = eglGetDisplay((EGLNativeDisplayType)x_display);
    if (mansion_display->egl_display == EGL_NO_DISPLAY) {
        std::cerr << "Failed to get EGL display" << std::endl;
        XDestroyWindow(x_display, x_window);
        XCloseDisplay(x_display);
        delete mansion_display;
        return nullptr;
    }

    EGLint major, minor;
    if (!eglInitialize(mansion_display->egl_display, &major, &minor)) {
        std::cerr << "Failed to initialize EGL" << std::endl;
        XDestroyWindow(x_display, x_window);
        XCloseDisplay(x_display);
        eglTerminate(mansion_display->egl_display);
        delete mansion_display;
        return nullptr;
    }

    if (!eglChooseConfig(mansion_display->egl_display, config_attr, &mansion_display->egl_config, 1,
                          &mansion_display->egl_config_count)) {
        std::cerr << "Failed to choose EGL config" << std::endl;
        XDestroyWindow(x_display, x_window);
        XCloseDisplay(x_display);
        eglTerminate(mansion_display->egl_display);
        delete mansion_display;
        return nullptr;
    }

    // Create EGL context
    mansion_display->egl_context = eglCreateContext(mansion_display->egl_display,
                                                     mansion_display->egl_config,
                                                     EGL_NO_CONTEXT, context_attr);
    if (mansion_display->egl_context == EGL_NO_CONTEXT) {
        std::cerr << "Failed to create EGL context" << std::endl;
        XDestroyWindow(x_display, x_window);
        XCloseDisplay(x_display);
        eglTerminate(mansion_display->egl_display);
        delete mansion_display;
        return nullptr;
    }

    // Create EGL window surface using X11
    mansion_display->egl_surface = eglCreateWindowSurface(mansion_display->egl_display,
                                                           mansion_display->egl_config,
                                                           (EGLNativeWindowType)x_window,
                                                           nullptr);
    if (mansion_display->egl_surface == EGL_NO_SURFACE) {
        std::cerr << "Failed to create EGL surface" << std::endl;
        eglDestroyContext(mansion_display->egl_display, mansion_display->egl_context);
        XDestroyWindow(x_display, x_window);
        XCloseDisplay(x_display);
        eglTerminate(mansion_display->egl_display);
        delete mansion_display;
        return nullptr;
    }

    // Make EGL context current
    if (eglMakeCurrent(mansion_display->egl_display, mansion_display->egl_surface,
                       mansion_display->egl_surface, mansion_display->egl_context) == EGL_FALSE) {
        std::cerr << "Failed to make EGL context current" << std::endl;
        eglDestroySurface(mansion_display->egl_display, mansion_display->egl_surface);
        eglDestroyContext(mansion_display->egl_display, mansion_display->egl_context);
        XDestroyWindow(x_display, x_window);
        XCloseDisplay(x_display);
        eglTerminate(mansion_display->egl_display);
        delete mansion_display;
        return nullptr;
    }

    // Set viewport
    glViewport(0, 0, mansion_display->window_width, mansion_display->window_height);

    // Initialize renderer
    if (!init_renderer(mansion_display)) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        eglDestroySurface(mansion_display->egl_display, mansion_display->egl_surface);
        eglDestroyContext(mansion_display->egl_display, mansion_display->egl_context);
        XDestroyWindow(x_display, x_window);
        XCloseDisplay(x_display);
        eglTerminate(mansion_display->egl_display);
        delete mansion_display;
        return nullptr;
    }

    return mansion_display;
}

struct MansionDisplay* create_display_headless(struct MansionCompositor* compositor, struct wl_display* wl_display) {
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
    mansion_display->x_display = nullptr;
    mansion_display->x_window  = None;

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

void display_resize(struct MansionDisplay* display) {
    if (!display || !display->renderer) return;
    glViewport(0, 0, display->window_width, display->window_height);
    glUniform2f(display->renderer->viewport_uniform,
                static_cast<GLfloat>(display->window_width),
                static_cast<GLfloat>(display->window_height));
}

void destroy_display(struct MansionDisplay* display) {
    if (!display) return;

    destroy_renderer(display);

    if (display->x_display) {
        if (display->x_window) {
            XDestroyWindow(display->x_display, display->x_window);
        }
        XCloseDisplay(display->x_display);
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

    glClearColor(0.15f, 0.15f, 0.2f, 1.0f);

    display->renderer = renderer;
    return true;
}

void destroy_renderer(struct MansionDisplay* display) {
    if (!display || !display->renderer) return;

    glUseProgram(0);
    glDeleteProgram(display->renderer->program);
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

void render(struct MansionDisplay* display) {
    if (!display) return;

    // Fire frame callbacks on every render tick (needed even in headless mode).
    fire_frame_callbacks(display->compositor);

    auto* renderer = display->renderer;
    if (!renderer) return;

    glClear(GL_COLOR_BUFFER_BIT);

    // Render all surfaces from the compositor
    struct MansionSurface *surface, *next;
    wl_list_for_each_safe(surface, next, &display->compositor->surface_list, link) {
        render_surface(display, surface->resource, 0, 0);
    }

    // Render orphaned surfaces (surfaces that survived client disconnect).
    // These no longer have a valid wl_resource, so render directly via surface pointer.
    wl_list_for_each_safe(surface, next, &display->compositor->orphaned_surfaces, link) {
        render_surface_from_data(display, surface, 0, 0);
    }

    swap_buffers(display);
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
    glUniform2f(renderer->viewport_uniform,
                static_cast<GLfloat>(display->window_width),
                static_cast<GLfloat>(display->window_height));

    if (surface_data->buffer_resource) {
        /* Upload buffer pixels as a GL texture. The source is ARGB8888 which on
         * little-endian is BGRA in memory. OpenGL's BGRA→RGBA conversion swaps
         * R↔B, so we swizzle on the CPU to get correct colours. */
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

                /* Swizzle BGRA → RGBA on CPU. */
                std::vector<uint8_t> rgba(w * h * 4);
                for (int y = 0; y < h; y++) {
                    for (int x = 0; x < w; x++) {
                        int idx = (y * w + x) * 4;
                        rgba[idx + 0] = ((const uint8_t*)data)[idx + 2]; // R
                        rgba[idx + 1] = ((const uint8_t*)data)[idx + 1]; // G
                        rgba[idx + 2] = ((const uint8_t*)data)[idx + 0]; // B
                        rgba[idx + 3] = ((const uint8_t*)data)[idx + 3]; // A
                    }
                }

                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
                             GL_UNSIGNED_BYTE, rgba.data());
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
    glUniform2f(renderer->viewport_uniform,
                static_cast<GLfloat>(display->window_width),
                static_cast<GLfloat>(display->window_height));

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

void swap_buffers(struct MansionDisplay* display) {
    if (display && display->egl_display != EGL_NO_DISPLAY &&
        display->egl_surface != EGL_NO_SURFACE) {
        eglSwapBuffers(display->egl_display, display->egl_surface);
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
