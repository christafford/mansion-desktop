#pragma once

#include <wayland-server.h>

struct MansionDisplay;
struct MansionCompositor;

/* Forward declaration - defined in compositor.cpp */
struct MansionSurface;

struct MansionCompositor* create_compositor(struct wl_display* display);
void destroy_compositor(struct MansionCompositor* compositor);

/* Get surface data from resource (for rendering) */
struct MansionSurface* compositor_surface_from_resource(struct wl_resource* resource);

/* Set surface dimensions (called when buffer is attached) */
void compositor_surface_set_size(struct MansionSurface* surface, int32_t width, int32_t height);
