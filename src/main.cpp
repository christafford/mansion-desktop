#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include <wayland-server.h>

#include "compositor.h"
#include "display.h"
#include "input.h"
#include "launch.h"
#include "shell.h"

static void handle_help(int argc, char** argv) {
    (void)argc; (void)argv;
    std::cout << "Usage: mansion-desktop [--launch=COMMAND]\n"
              << "Launch a Wayland client after the compositor starts.\n"
              << "Example: mansion-desktop --launch=weston-terminal\n"
              << std::endl;
    std::exit(0);
}

static std::string get_socket_name() {
    const char* socket_name = getenv("WAYLAND_DISPLAY");
    if (socket_name) {
        return socket_name;
    }
    return "mansion-desktop";
}

static volatile sig_atomic_t running = 1;

static void signal_handler(int signum) {
    (void)signum;
    running = 0;
}

int main(int argc, char** argv) {
    std::string launch_cmd;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            handle_help(argc, argv);
        } else if (arg.find("--launch=") == 0) {
            launch_cmd = arg.substr(9);
            if (launch_cmd.empty()) {
                std::cerr << "Error: --launch= must be followed by a command\n" << std::endl;
                return 1;
            }
        } else {
            std::cerr << "Error: unknown argument " << arg << "\n" << std::endl;
            return 1;
        }
    }

    struct wl_display* wl_display = wl_display_create();
    if (!wl_display) {
        std::cerr << "Failed to create Wayland display" << std::endl;
        return 1;
    }

    auto* event_loop = wl_display_get_event_loop(wl_display);

    // Create compositor (needs wl_display)
    auto* compositor = create_compositor(wl_display);
    if (!compositor) {
        std::cerr << "Failed to create compositor" << std::endl;
        wl_display_destroy(wl_display);
        return 1;
    }

    // Create EGL display (needs compositor and wl_display)
    auto* egl_display = create_display(compositor, wl_display);
    if (!egl_display) {
        std::cerr << "Failed to create EGL display" << std::endl;
        destroy_compositor(compositor);
        wl_display_destroy(wl_display);
        return 1;
    }

    // Create shell (needs compositor and wl_display)
    auto* shell = create_shell(compositor, wl_display);
    if (!shell) {
        std::cerr << "Failed to create shell" << std::endl;
        destroy_display(egl_display);
        destroy_compositor(compositor);
        wl_display_destroy(wl_display);
        return 1;
    }

    // Create seat/input (needs wl_display)
    auto* seat = create_seat(wl_display);
    if (!seat) {
        std::cerr << "Failed to create seat" << std::endl;
        destroy_shell(shell);
        destroy_display(egl_display);
        destroy_compositor(compositor);
        wl_display_destroy(wl_display);
        return 1;
    }

    // Add socket
    const char* socket_name = get_socket_name().c_str();
    if (wl_display_add_socket(wl_display, socket_name) < 0) {
        std::cerr << "Failed to add socket to display" << std::endl;
        destroy_seat(seat);
        destroy_shell(shell);
        destroy_display(egl_display);
        destroy_compositor(compositor);
        wl_display_destroy(wl_display);
        return 1;
    }

    // Initialize shared memory
    wl_display_init_shm(wl_display);

    // Initialize evdev input
    if (input_init() < 0) {
        std::cerr << "Warning: Failed to initialize input devices" << std::endl;
    }

    // Launch application if requested
    if (!launch_cmd.empty()) {
        auto* app = launch_app(wl_display, launch_cmd.c_str(), launch_cmd.c_str());
        if (app) {
            std::cerr << "Launched: " << launch_cmd << " (PID: " << app_pid(app) << ")" << std::endl;
        } else {
            std::cerr << "Failed to launch app: " << launch_cmd << std::endl;
        }
    }

    // Set up signal handling for clean exit
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);

    std::cerr << "Mansion Desktop running on socket: " << socket_name << std::endl;

    // Run event loop
    int frame_count = 0;
    while (running) {
        // Dispatch Wayland events with a short timeout to allow periodic rendering
        int result = wl_event_loop_dispatch(event_loop, 16);  // ~60fps
        if (result < 0) {
            std::cerr << "Event loop error: " << result << std::endl;
            break;
        }

        // Flush pending writes to all connected clients
        wl_display_flush_clients(wl_display);

        // Process input events
        input_process();

        // Render frame
        render(egl_display);
        frame_count++;
        if (frame_count % 30 == 0) {
            std::cerr << "Frame " << frame_count << std::endl;
        }
    }

    std::cerr << "Mansion Desktop exiting (frames: " << frame_count << ")" << std::endl;

    // Cleanup - destroy in reverse order
    input_destroy();
    destroy_seat(seat);
    destroy_shell(shell);
    destroy_display(egl_display);
    destroy_compositor(compositor);
    wl_display_destroy(wl_display);

    return 0;
}
