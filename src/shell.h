#pragma once

#include <wayland-server.h>

struct MansionCompositor;
struct MansionShell;

struct MansionShell* create_shell(struct MansionCompositor* compositor, struct wl_display* display);
void destroy_shell(struct MansionShell* shell);