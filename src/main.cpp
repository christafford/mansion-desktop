#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <sys/param.h>

#include <wayland-server.h>

#include "compositor.h"
#include "shell.h"

struct MansionDesktop {
    struct wl_display* display;
    struct wl_event_loop* event_loop;
    uint32_t serial;

    struct MansionCompositor* compositor;
    struct MansionShell* shell;

    bool should_run = true;
};

static void handle_help(int argc, char** argv) {
    (void)argc;
    (void)argv;
    std::cout << "Usage: mansion-desktop [--launch=COMMAND]\n"
              << "Launch a Wayland client after the compositor starts.\n"
              << "Example: mansion-desktop --launch=weston-terminal\n"
              << std::endl;
    exit(0);
}

static std::string get_socket_name() {
    const char* socket_name = getenv("WAYLAND_DISPLAY");
    if (socket_name) {
        return socket_name;
    }
    return "mansion-desktop";
}



int main(int argc, char** argv) {
    std::vector<char*> args;
    args.push_back(argv[0]);
    std::string launch_app;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            handle_help(argc, argv);
        } else if (arg.find("--launch=") == 0) {
            launch_app = arg.substr(9);
            if (launch_app.empty()) {
                std::cerr << "Error: --launch= must be followed by a command\n"
                          << std::endl;
                return 1;
            }
        } else {
            std::cerr << "Error: unknown argument " << arg << "\n"
                      << std::endl;
            return 1;
        }
    }

    struct MansionDesktop mansion = {0, nullptr, 0};
    mansion.serial = 1;

    mansion.event_loop = wl_event_loop_create();
    if (!mansion.event_loop) {
        std::cerr << "Failed to create event loop\n" << std::endl;
        return 1;
    }

    mansion.display = wl_display_create();
    if (!mansion.display) {
        std::cerr << "Failed to create Wayland display\n" << std::endl;
        wl_event_loop_destroy(mansion.event_loop);
        return 1;
    }

    // TODO: Set up compositor, shell, etc.
    // TODO: Set up EGL and renderer
    // TODO: Set up input devices

    const char* socket_name = get_socket_name().c_str();
    if (wl_display_add_socket(mansion.display, socket_name) < 0) {
        std::cerr << "Failed to add socket to display\n" << std::endl;
        wl_event_loop_destroy(mansion.event_loop);
        wl_display_destroy(mansion.display);
        return 1;
    }

    wl_display_init_shm(mansion.display);

    mansion.compositor = create_compositor(mansion.display);
    if (!mansion.compositor) {
        std::cerr << "Failed to initialize compositor\n" << std::endl;
        wl_display_destroy(mansion.display);
        wl_event_loop_destroy(mansion.event_loop);
        return 1;
    }

    mansion.shell = create_shell(mansion.compositor, mansion.display);
    if (!mansion.shell) {
        std::cerr << "Failed to initialize shell\n" << std::endl;
        destroy_compositor(mansion.compositor);
        wl_display_destroy(mansion.display);
        wl_event_loop_destroy(mansion.event_loop);
        return 1;
    }

    const char* socket_name = get_socket_name().c_str();
    if (wl_display_add_socket(mansion.display, socket_name) < 0) {
        std::cerr << "Failed to add socket to display\n" << std::endl;
        destroy_shell(mansion.shell);
        destroy_compositor(mansion.compositor);
        wl_display_destroy(mansion.display);
        wl_event_loop_destroy(mansion.event_loop);
        return 1;
    }

    wl_display_run(mansion.display);

    destroy_shell(mansion.shell);
    destroy_compositor(mansion.compositor);
    wl_display_destroy(mansion.display);
    wl_event_loop_destroy(mansion.event_loop);

    if (!launch_app.empty()) {
        std::cout << "Note: Launching clients is not yet implemented\n" << std::endl;
    }

    return 0;
}
