#pragma once

#include <sys/types.h>
#include <wayland-server.h>

struct MansionApp;

/* Fork and exec `command` (whitespace-split) with WAYLAND_DISPLAY set to `socket_name`.
 * Returns nullptr if the process could not be started. The child is not reaped
 * automatically yet; destroy_app terminates it if it is still running. */
MansionApp* launch_app(struct wl_display* display, const char* socket_name, const char* command);
void destroy_app(struct MansionApp* app);
pid_t app_pid(struct MansionApp* app);
bool app_has_pid(struct MansionApp* app);
