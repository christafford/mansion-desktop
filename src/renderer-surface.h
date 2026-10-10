#pragma once

#include <GLES2/gl2.h>

/* Rendering-layer per-surface state.
 *
 * This struct owns all GPU resources for a Wayland surface.  It lives
 * entirely in the presentation layer (display.cpp) so that the
 * compositor core can be used without any GL/EGL dependencies.
 *
 * The bridge (P21-T11) queries surface state through display.h
 * accessor functions rather than reaching into this struct.
 */
struct ElsewhereRendererSurface {
    GLuint gl_texture = 0;  /* 0 = no texture uploaded yet */
};
