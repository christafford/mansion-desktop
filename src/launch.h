#pragma once

#include <sys/types.h>
#include <wayland-server.h>

struct ElsewhereApp;

/* Fork and exec `command` (whitespace-split) with WAYLAND_DISPLAY set to `socket_name`.
 * Returns nullptr if the process could not be started. The child is reaped
 * automatically via SIGCHLD in the main event loop; destroy_app terminates
 * it if it is still running. */
ElsewhereApp* launch_app(struct wl_display* display, const char* socket_name, const char* command);
void destroy_app(struct ElsewhereApp* app);
pid_t app_pid(struct ElsewhereApp* app);
bool app_has_pid(struct ElsewhereApp* app);
