#pragma once

#include <wayland-server.h>

struct MansionCompositor;

struct MansionCompositor* create_compositor(struct wl_display* display);
void destroy_compositor(struct MansionCompositor* compositor);