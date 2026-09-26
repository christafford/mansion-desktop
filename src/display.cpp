#include <cstring>
#include <iostream>
#include <string>
#include <utility>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <wayland-server-protocol.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include "display.h"
#include "compositor.h"
#include "compositor-private.h"

struct MansionRenderer {
    GLuint program;
    GLint pos_attrib;
    GLint tex_attrib;
    GLint color_uniform;
    GLint tex_uniform;
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

    // Store X11 display and window - keep alive for EGL lifetime
    auto* x11_data = new std::pair<Display*, Window>(x_display, x_window);
    XSetWindowBorderWidth(x_display, x_window, 2);
    Colormap border_color = XCreateColormap(x_display, DefaultRootWindow(x_display),
                                            DefaultVisual(x_display, screen), AllocNone);
    XSetWindowBorder(x_display, x_window, border_color);
    XFlush(x_display);

    // Create EGL display for X11
    mansion_display->egl_display = eglGetDisplay((EGLNativeDisplayType)x_display);
    if (mansion_display->egl_display == EGL_NO_DISPLAY) {
        std::cerr << "Failed to get EGL display" << std::endl;
        XDestroyWindow(x_display, x_window);
        XCloseDisplay(x_display);
        delete x11_data;
        delete mansion_display;
        return nullptr;
    }

    EGLint major, minor;
    if (!eglInitialize(mansion_display->egl_display, &major, &minor)) {
        std::cerr << "Failed to initialize EGL" << std::endl;
        XDestroyWindow(x_display, x_window);
        XCloseDisplay(x_display);
        eglTerminate(mansion_display->egl_display);
        delete x11_data;
        delete mansion_display;
        return nullptr;
    }

    if (!eglChooseConfig(mansion_display->egl_display, config_attr, &mansion_display->egl_config, 1,
                          &mansion_display->egl_config_count)) {
        std::cerr << "Failed to choose EGL config" << std::endl;
        XDestroyWindow(x_display, x_window);
        XCloseDisplay(x_display);
        eglTerminate(mansion_display->egl_display);
        delete x11_data;
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
        delete x11_data;
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
        delete x11_data;
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
        delete x11_data;
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
        delete x11_data;
        delete mansion_display;
        return nullptr;
    }

    return mansion_display;
}

void destroy_display(struct MansionDisplay* display) {
    if (!display) return;

    destroy_renderer(display);

    if (display->egl_display != EGL_NO_DISPLAY) {
        eglMakeCurrent(display->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroySurface(display->egl_display, display->egl_surface);
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
        "attribute vec2 pos;\n"
        "attribute vec2 texcoord;\n"
        "varying vec2 v_texcoord;\n"
        "void main() {\n"
        "    gl_Position = vec4(pos, 0.0, 1.0);\n"
        "    v_texcoord = texcoord;\n"
        "}\n";

    static const char* fragment_shader_source =
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

void render(struct MansionDisplay* display) {
    auto* renderer = display->renderer;
    if (!renderer) return;

    glClear(GL_COLOR_BUFFER_BIT);

    // Render all surfaces from the compositor
    struct MansionSurface *surface, *next;
    wl_list_for_each_safe(surface, next, &display->compositor->surface_list, link) {
        render_surface(display, surface->resource, 0, 0);
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

    glUseProgram(renderer->program);

    if (surface_data->buffer_resource) {
        // Render with texture (TODO: actually bind the buffer texture)
        float sw = 200.0f;
        float sh = 150.0f;

        GLfloat attrib_data[] = {
            static_cast<GLfloat>(sx),     static_cast<GLfloat>(sy),       0.0f, 0.0f,
            static_cast<GLfloat>(sx) + sw, static_cast<GLfloat>(sy),       1.0f, 0.0f,
            static_cast<GLfloat>(sx),     static_cast<GLfloat>(sy) + sh, 0.0f, 1.0f,
            static_cast<GLfloat>(sx) + sw, static_cast<GLfloat>(sy) + sh, 1.0f, 1.0f,
        };

        glEnableVertexAttribArray(renderer->pos_attrib);
        glVertexAttribPointer(renderer->pos_attrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), &attrib_data[0]);

        glEnableVertexAttribArray(renderer->tex_attrib);
        glVertexAttribPointer(renderer->tex_attrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), &attrib_data[2]);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUniform1i(renderer->tex_uniform, 0);
        glUniform4f(renderer->color_uniform, 1.0f, 1.0f, 1.0f, 0.0f);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glDisableVertexAttribArray(renderer->pos_attrib);
        glDisableVertexAttribArray(renderer->tex_attrib);
    } else {
        // Render solid color placeholder
        float sw = 200.0f;
        float sh = 150.0f;

        GLfloat attrib_data[] = {
            static_cast<GLfloat>(sx),     static_cast<GLfloat>(sy),
            static_cast<GLfloat>(sx) + sw, static_cast<GLfloat>(sy),
            static_cast<GLfloat>(sx),     static_cast<GLfloat>(sy) + sh,
            static_cast<GLfloat>(sx) + sw, static_cast<GLfloat>(sy) + sh,
        };

        glEnableVertexAttribArray(renderer->pos_attrib);
        glVertexAttribPointer(renderer->pos_attrib, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), attrib_data);
        glDisableVertexAttribArray(renderer->tex_attrib);

        glUniform4f(renderer->color_uniform, 0.3f, 0.3f, 0.4f, 1.0f);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glDisableVertexAttribArray(renderer->pos_attrib);
    }
}

void swap_buffers(struct MansionDisplay* display) {
    if (display && display->egl_display != EGL_NO_DISPLAY) {
        eglSwapBuffers(display->egl_display, display->egl_surface);
    }
}
