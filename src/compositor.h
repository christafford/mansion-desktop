#pragma once

#include <wayland-server.h>

struct ElsewhereDisplay;
struct ElsewhereCompositor;

/* Forward declaration - defined in compositor.cpp */
struct ElsewhereSurface;

struct ElsewhereCompositor* create_compositor(struct wl_display* display);
void destroy_compositor(struct ElsewhereCompositor* compositor);

/* Get surface data from resource (for rendering) */
struct ElsewhereSurface* compositor_surface_from_resource(struct wl_resource* resource);

/* Set surface dimensions (called when buffer is attached) */
void compositor_surface_set_size(struct ElsewhereSurface* surface, int32_t width, int32_t height);

/* Find surface by client serial (for snapshot creation) */
struct ElsewhereSurface* compositor_surface_from_serial(struct ElsewhereCompositor* compositor, uint32_t serial);
