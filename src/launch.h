#pragma once

#include <wayland-server.h>

struct MansionApp;

MansionApp* launch_app(struct wl_display* display, const char* executable, const char* name);
void destroy_app(struct MansionApp* app);
pid_t app_pid(struct MansionApp* app);
bool app_has_pid(struct MansionApp* app);
