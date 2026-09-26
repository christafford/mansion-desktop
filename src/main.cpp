#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include <unistd.h>
#include <wayland-server.h>

#include "compositor.h"
#include "display.h"
#include "input.h"
#include "launch.h"
#include "shell.h"

namespace {

struct Options {
    std::vector<std::string> launch;
    std::string socket;      // empty: mansion-<pid>
    bool headless = false;   // no host window, no EGL, no host input
    long exit_after_ms = -1; // <0: run until a signal arrives
};

void print_help() {
    std::cout <<
        "Usage: mansion-desktop [options]\n"
        "  --launch CMD         launch a Wayland client on the private socket (repeatable)\n"
        "  --launch=CMD         same as above\n"
        "  --socket NAME        Wayland socket name (default: mansion-<pid>)\n"
        "  --headless           no host window or input; for automated tests\n"
        "  --exit-after-ms N    exit with status 0 after N milliseconds\n"
        "  -h, --help           show this help\n"
        "The socket name is printed to stdout as MANSION_SOCKET=<name>.\n";
}

// Returns true when parsing succeeded. `--flag=value` and `--flag value` are both accepted.
bool parse_args(int argc, char** argv, Options& opts) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        std::string name = arg, value;
        bool has_value = false;
        auto eq = arg.find('=');
        if (arg.rfind("--", 0) == 0 && eq != std::string::npos) {
            name = arg.substr(0, eq);
            value = arg.substr(eq + 1);
            has_value = true;
        }
        auto take_value = [&]() -> bool {
            if (has_value) return true;
            if (i + 1 >= argc) {
                std::cerr << "Error: " << name << " requires a value\n";
                return false;
            }
            value = argv[++i];
            has_value = true;
            return true;
        };

        if (name == "--help" || name == "-h") {
            print_help();
            std::exit(0);
        } else if (name == "--launch") {
            if (!take_value()) return false;
            if (value.empty()) {
                std::cerr << "Error: --launch requires a command\n";
                return false;
            }
            opts.launch.push_back(value);
        } else if (name == "--socket") {
            if (!take_value()) return false;
            if (value.empty() || value.find('/') != std::string::npos) {
                std::cerr << "Error: --socket needs a plain name without '/'\n";
                return false;
            }
            opts.socket = value;
        } else if (name == "--headless") {
            opts.headless = true;
        } else if (name == "--exit-after-ms") {
            if (!take_value()) return false;
            char* end = nullptr;
            long n = std::strtol(value.c_str(), &end, 10);
            if (!end || *end != '\0' || n < 0) {
                std::cerr << "Error: --exit-after-ms needs a non-negative integer\n";
                return false;
            }
            opts.exit_after_ms = n;
        } else {
            std::cerr << "Error: unknown argument " << arg << "\n";
            return false;
        }
    }
    return true;
}

volatile sig_atomic_t running = 1;

void signal_handler(int) {
    running = 0;
}

} // namespace

int main(int argc, char** argv) {
    Options opts;
    if (!parse_args(argc, argv, opts)) {
        return 2;
    }

    // Never reuse the host compositor's socket name: this process is nested inside it.
    const std::string socket_name =
        opts.socket.empty() ? "mansion-" + std::to_string(getpid()) : opts.socket;

    if (!getenv("XDG_RUNTIME_DIR")) {
        std::cerr << "XDG_RUNTIME_DIR is not set; libwayland cannot create a socket" << std::endl;
        return 1;
    }

    struct wl_display* wl_display = wl_display_create();
    if (!wl_display) {
        std::cerr << "Failed to create Wayland display" << std::endl;
        return 1;
    }
    auto* event_loop = wl_display_get_event_loop(wl_display);

    MansionCompositor* compositor = nullptr;
    MansionDisplay* display = nullptr;
    MansionShell* shell = nullptr;
    MansionSeat* seat = nullptr;
    std::vector<MansionApp*> apps;
    int status = 1;
    bool input_started = false;

    auto cleanup = [&]() {
        for (auto* app : apps) destroy_app(app);
        apps.clear();
        if (input_started) input_destroy();
        destroy_seat(seat);
        destroy_shell(shell);
        destroy_display(display);
        destroy_compositor(compositor);
        // wl_display_destroy removes the socket and lock file that wl_display_add_socket created.
        wl_display_destroy(wl_display);
    };

    compositor = create_compositor(wl_display);
    if (!compositor) {
        std::cerr << "Failed to create compositor" << std::endl;
        cleanup();
        return 1;
    }

    display = opts.headless ? create_display_headless(compositor, wl_display)
                            : create_display(compositor, wl_display);
    if (!display) {
        std::cerr << "Failed to create " << (opts.headless ? "headless" : "EGL") << " display" << std::endl;
        cleanup();
        return 1;
    }

    shell = create_shell(compositor, wl_display);
    if (!shell) {
        std::cerr << "Failed to create shell" << std::endl;
        cleanup();
        return 1;
    }

    seat = create_seat(wl_display);
    if (!seat) {
        std::cerr << "Failed to create seat" << std::endl;
        cleanup();
        return 1;
    }

    if (wl_display_add_socket(wl_display, socket_name.c_str()) < 0) {
        std::cerr << "Failed to add socket '" << socket_name << "': " << strerror(errno) << std::endl;
        cleanup();
        return 1;
    }

    wl_display_init_shm(wl_display);

    if (!opts.headless) {
        if (input_init() < 0) {
            std::cerr << "Warning: failed to initialize input devices" << std::endl;
        } else {
            input_started = true;
        }
    }

    // Scripts read this line to find the socket. Print it only once everything is listening.
    std::cout << "MANSION_SOCKET=" << socket_name << std::endl;
    std::cerr << "Mansion Desktop running on socket: " << socket_name
              << (opts.headless ? " (headless)" : "") << std::endl;

    for (const auto& cmd : opts.launch) {
        auto* app = launch_app(wl_display, socket_name.c_str(), cmd.c_str());
        if (app) {
            std::cerr << "Launched: " << cmd << " (PID: " << app_pid(app) << ")" << std::endl;
            apps.push_back(app);
        } else {
            std::cerr << "Failed to launch app: " << cmd << std::endl;
        }
    }

    struct sigaction sa {};
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    signal(SIGPIPE, SIG_IGN);

    using clock = std::chrono::steady_clock;
    const auto start = clock::now();
    long frame_count = 0;
    status = 0;

    while (running) {
        if (opts.exit_after_ms >= 0) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - start).count();
            if (elapsed >= opts.exit_after_ms) break;
        }

        // Dispatch Wayland events with a short timeout so rendering runs at roughly 60 Hz.
        if (wl_event_loop_dispatch(event_loop, 16) < 0) {
            std::cerr << "Event loop error: " << strerror(errno) << std::endl;
            status = 1;
            break;
        }
        wl_display_flush_clients(wl_display);

        if (input_started) input_process();

        render(display);
        frame_count++;
    }

    std::cerr << "Mansion Desktop exiting (frames: " << frame_count << ")" << std::endl;
    cleanup();
    return status;
}
