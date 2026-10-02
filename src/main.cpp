#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

#include <sys/signalfd.h>
#include <sys/wait.h>
#include <unistd.h>
#include <wayland-client.h>
#include <wayland-server.h>

#include "compositor.h"
#include "display.h"
#include "host-events.h"
#include "input.h"
#include "launch.h"
#include "xdg-shell.h"

namespace {

/* Client Wayland display — events from the real compositor must be
 * dispatched from the event loop so input and configure events are
 * processed. */
static struct wl_display* g_client_display = nullptr;
static struct wl_event_source* g_client_display_source = nullptr;

struct Options {
    std::vector<std::string> launch;
    std::string socket;      // empty: mansion-<pid>
    bool headless = false;   // no host window, no EGL, no host input
    long exit_after_ms = -1; // <0: run until a signal arrives
    std::string screenshot;  // if set, write a PPM screenshot after the loop
    std::string input_script; // if set, execute scripted input events
    bool flat = false;       // P2-T02: 2D rendering instead of 3D panel
    bool stats = false;      // P2-T07: print frame timing / bytes uploaded
    int world_key = 88;      // P3-T02: evdev keycode for world-key (default F12=88)
    int tab_key = 23;        // P5-T02: evdev keycode for tab key (default Tab=23)
    float camera[5] = {0, 0, 10, 0, 0}; // X,Y,Z,yaw,pitch for P2-T02
    bool camera_specified = false; // true if --camera was explicitly passed
    bool room_camera = false;    // P4-T01: default room camera position
};

void print_help() {
    std::cout <<
        "Usage: mansion-desktop [options]\n"
        "  --launch CMD         launch a Wayland client on the private socket (repeatable)\n"
        "  --launch=CMD         same as above\n"
        "  --socket NAME        Wayland socket name (default: mansion-<pid>)\n"
        "  --headless           no host window or input; for automated tests\n"
        "  --flat               2D rendering (disable 3D perspective panel, P2-T02)\n"
        "  --camera X,Y,Z,YAW,PITCH  camera position + orientation for 3D panel (P2-T02)\n"
        "  --exit-after-ms N    exit with status 0 after N milliseconds\n"
        "  --screenshot FILE    write a binary PPM screenshot after the loop\n"
        "  --input-script FILE  execute scripted input events during the loop\n"
        "  --stats              print per-frame render time and bytes uploaded every 60 frames (P2-T07)\n"
        "  --world-key N        evdev keycode for world-mode shortcut (default: 88=F12, P3-T02)\n"
        "  --tab-key N          evdev keycode for tab (default: 23=Tab, P5-T02)\n"
        "  --room-camera        use room-mode camera (inside a 3D room, P4-T01)\n"
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
        } else if (name == "--flat") {
            opts.flat = true;
        } else if (name == "--camera") {
            if (!take_value()) return false;
            if (value.empty()) {
                std::cerr << "Error: --camera needs X,Y,Z,YAW,PITCH\n";
                return false;
            }
            /* Parse "X,Y,Z,YAW,PITCH" — all float values. */
            std::vector<float> vals;
            std::string token;
            for (char c : value) {
                if (c == ',') {
                    if (!token.empty()) {
                        vals.push_back(std::stof(token));
                        token.clear();
                    }
                } else {
                    token += c;
                }
            }
            if (!token.empty()) vals.push_back(std::stof(token));
            if (vals.size() < 3) {
                std::cerr << "Error: --camera needs at least X,Y,Z\n";
                return false;
            }
            opts.camera[0] = vals[0]; // X
            opts.camera[1] = vals[1]; // Y
            opts.camera[2] = vals[2]; // Z
            opts.camera[3] = vals.size() > 3 ? vals[3] : 0;   // yaw
            opts.camera[4] = vals.size() > 4 ? vals[4] : 0;   // pitch
            opts.camera_specified = true;
        } else if (name == "--exit-after-ms") {
            if (!take_value()) return false;
            char* end = nullptr;
            long n = std::strtol(value.c_str(), &end, 10);
            if (!end || *end != '\0' || n < 0) {
                std::cerr << "Error: --exit-after-ms needs a non-negative integer\n";
                return false;
            }
            opts.exit_after_ms = n;
        } else if (name == "--screenshot") {
            if (!take_value()) return false;
            if (value.empty()) {
                std::cerr << "Error: --screenshot requires a path\n";
                return false;
            }
            opts.screenshot = value;
        } else if (name == "--input-script") {
            if (!take_value()) return false;
            if (value.empty()) {
                std::cerr << "Error: --input-script requires a file path\n";
                return false;
            }
            opts.input_script = value;
        } else if (name == "--stats") {
            opts.stats = true;
        } else if (name == "--world-key") {
            if (!take_value()) return false;
            opts.world_key = (int)strtol(value.c_str(), nullptr, 10);
        } else if (name == "--tab-key") {
            if (!take_value()) return false;
            opts.tab_key = (int)strtol(value.c_str(), nullptr, 10);
        } else if (name == "--room-camera") {
            /* P4-T01: room mode camera (inside the room, looking at front wall). */
            opts.room_camera = true;
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

// ── Client lifecycle logging ──────────────────────────────────────────────

static void on_client_destroyed(struct wl_listener* listener, void* data) {
    struct wl_client* client = static_cast<struct wl_client*>(data);
    pid_t pid = 0;
    wl_client_get_credentials(client, &pid, nullptr, nullptr);
    std::cerr << "client disconnected " << pid << std::endl;
    wl_list_remove(&listener->link);
    delete listener;
}

static void on_client_created(struct wl_listener* listener, void* data) {
    (void)listener;
    struct wl_client* client = static_cast<struct wl_client*>(data);
    pid_t pid = 0;
    wl_client_get_credentials(client, &pid, nullptr, nullptr);
    std::cerr << "client connected " << pid << std::endl;

    auto* l = new struct wl_listener;
    l->notify = on_client_destroyed;
    wl_list_init(&l->link);
    wl_client_add_destroy_listener(client, l);
}

// ── SIGCHLD reaping ───────────────────────────────────────────────────────

static int g_sigchld_fd = -1;
static struct wl_event_source* g_sigchld_source = nullptr;

static int sigchld_handler(int fd, uint32_t /*mask*/, void* /*data*/) {
    struct signalfd_siginfo si;
    ssize_t s = read(fd, &si, sizeof(si));
    if (s < static_cast<ssize_t>(sizeof(si))) return 0;

    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        int exit_code = 0;
        if (WIFEXITED(status)) {
            exit_code = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            exit_code = 128 + WTERMSIG(status);
        }
        std::cerr << "child " << pid << " exited " << exit_code << std::endl;
    }
    return 0;
}

static void setup_sigchld(struct wl_event_loop* event_loop) {
    // Block SIGCHLD so signalfd receives it.
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, nullptr);

    g_sigchld_fd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
    if (g_sigchld_fd >= 0) {
        g_sigchld_source = wl_event_loop_add_fd(event_loop, g_sigchld_fd, WL_EVENT_READABLE,
                                                sigchld_handler, nullptr);
    }
}

// ── SIGSEGV handler for crash debugging ──────────────────────────────────

static void sigsegv_handler(int /*sig*/, siginfo_t* info, void* ucontext) {
    if (!ucontext) { _exit(139); }
    ucontext_t* uc = (ucontext_t*)ucontext;
    mcontext_t& mc = uc->uc_mcontext;

    // Get RIP from context
    #ifdef __x86_64__
    uintptr_t rip = mc.gregs[REG_RIP];
    uintptr_t rax = mc.gregs[REG_RAX];
    uintptr_t rdi = mc.gregs[REG_RDI];
    uintptr_t rsi = mc.gregs[REG_RSI];
    uintptr_t rbx = mc.gregs[REG_RBX];
    uintptr_t rsp = mc.gregs[REG_RSP];
    #else
    uintptr_t rip = 0, rax = 0, rdi = 0, rsi = 0, rbx = 0, rsp = 0;
    #endif

    std::cerr << "[CRASH] RIP=" << (void*)rip
              << " si_addr=" << info->si_addr
              << " RAX=" << (void*)rax
              << " RDI=" << (void*)rdi
              << " RSI=" << (void*)rsi
              << " RBX=" << (void*)rbx
              << " rsp=" << (void*)rsp
              << std::endl;

    // Dump 64 uint64_t values around si_addr
    uint64_t* addr64 = (uint64_t*)((unsigned char*)info->si_addr - 8);
    std::cerr << "[CRASH] uint64 dump around si_addr (si_addr = addr64[1]):" << std::endl;
    for (int i = 0; i < 10; i++) {
        const char* tag = "";
        if (i == 0) tag = " [base]";
        if (i == 1) tag = " [listener]";
        if (i == 3) tag = " [notify]";
        std::cerr << "  [" << i << "]" << tag << " = " << (void*)addr64[i] << std::endl;
    }
    // Also dump the full 32-byte pkc2 struct
    uint64_t* base = addr64;
    std::cerr << "[CRASH] pkc2 struct (32 bytes = 4 uint64_t):" << std::endl;
    std::cerr << "  resource     = " << (void*)base[0] << std::endl;
    std::cerr << "  link.prev    = " << (void*)base[1] << std::endl;
    std::cerr << "  link.next    = " << (void*)base[2] << std::endl;
    std::cerr << "  notify       = " << (void*)base[3] << std::endl;
    _exit(139);
}

static void install_sigsegv_handler() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = sigsegv_handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, nullptr);
}

int main(int argc, char** argv) {
    Options opts;
    if (!parse_args(argc, argv, opts)) {
        return 2;
    }

    install_sigsegv_handler();

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

    // Set up SIGCHLD reaping and client lifecycle logging.
    setup_sigchld(event_loop);

    struct wl_listener client_created_listener;
    client_created_listener.notify = on_client_created;
    wl_list_init(&client_created_listener.link);
    wl_display_add_client_created_listener(wl_display, &client_created_listener);

    MansionCompositor* compositor = nullptr;
    MansionDisplay* display = nullptr;
    MansionXdgShell* xdg_shell = nullptr;
    MansionSeat* seat = nullptr;
    std::vector<MansionApp*> apps;
    int status = 1;
    bool input_started = false;

    auto cleanup = [&]() {
        for (auto* app : apps) destroy_app(app);
        apps.clear();
        if (g_sigchld_fd >= 0) {
            close(g_sigchld_fd);
            g_sigchld_fd = -1;
        }
        if (g_sigchld_source) {
            wl_event_source_remove(g_sigchld_source);
            g_sigchld_source = nullptr;
        }
        if (g_client_display_source) {
            wl_event_source_remove(g_client_display_source);
            g_client_display_source = nullptr;
        }
        if (input_started) input_destroy();
        destroy_seat(seat);
        destroy_xdg_shell(xdg_shell);
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

    g_compositor = compositor;

    display = opts.headless ? create_display_headless(compositor, wl_display)
                            : create_display(compositor, wl_display);
    if (!display) {
        std::cerr << "Failed to create " << (opts.headless ? "headless" : "EGL") << " display" << std::endl;
        cleanup();
        return 1;
    }

    /* P3-T01: register the real Wayland client display FD so that
     * input events, configure events, and buffer-release events from
     * the host compositor are dispatched each frame. */
    if (!opts.headless && display->wl_client_display) {
        g_client_display = display->wl_client_display;
        int fd = wl_display_get_fd(g_client_display);
        g_client_display_source = wl_event_loop_add_fd(event_loop, fd,
                        WL_EVENT_READABLE, client_display_fd_handler, g_client_display);
    }

    /* P2-T02: configure 3D panel rendering.
     * Default to 3D world mode; flat mode only when --flat is given or
     * when headless testing needs 2D backward compatibility. */
    display->flat_mode = opts.headless && !opts.camera_specified;
    if (opts.flat)
        display->flat_mode = true;
    /* P2-T07: frame timing stats */
    display->stats_enabled = opts.stats;
    if (opts.room_camera) {
        /* P4-T01: room mode — camera inside the room looking at front wall. */
        display->room_mode = true;
        display->camera.x = 0;
        display->camera.y = 2.5f;
        display->camera.z = 3;
        display->camera.yaw = 0;
        display->camera.pitch = -10.0f * 3.14159265f / 180.0f;
        display->camera.fov = 1.5708f;
        display->panel.x = 0;
        display->panel.y = 3.0f;
        display->panel.z = -5;
        display->panel.width = 2.0f;
        display->panel.height = 1.5f;
        display->flat_mode = false;

        /* P4-T02: teleport target — closer to monitor, facing it. */
        display->teleport_x = 0;
        display->teleport_y = 3.0f;
        display->teleport_z = 0;
        display->teleport_yaw = 0;
        display->teleport_pitch = 0;

        /* P4-T02: set teleport key (T = evdev 20). */
        input_teleport_key_set(20);
    } else if (opts.camera_specified) {
        display->camera.x = opts.camera[0];
        display->camera.y = opts.camera[1];
        display->camera.z = opts.camera[2];
        display->camera.yaw = opts.camera[3];
        display->camera.pitch = opts.camera[4];
        display->panel.width = 2.0f;
        display->panel.height = 1.5f;
    }

    xdg_shell = create_xdg_shell(compositor, wl_display);
    if (!xdg_shell) {
        std::cerr << "Failed to create xdg-shell" << std::endl;
        cleanup();
        return 1;
    }

    seat = create_seat(wl_display);
    if (!seat) {
        std::cerr << "Failed to create seat" << std::endl;
        cleanup();
        return 1;
    }

    /* Connect seat to compositor so focus tracking can send enter/leave. */
    compositor_set_seat(compositor, seat);
    /* P3-T02: configure world-key shortcut. */
    input_world_key_set(opts.world_key);
    /* P5-T02: configure tab-key shortcut. */
    input_tab_key_set(opts.tab_key);

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

    /* Execute input script if one was provided. */
    struct InputScript* script = nullptr;
    if (!opts.input_script.empty()) {
        script = input_script_init(opts.input_script.c_str());
        if (!script) {
            std::cerr << "Failed to open input script: " << opts.input_script << std::endl;
            status = 1;
        }
    }

    // Scripts and test harnesses read this line to find the socket.
    // Print it only after the main loop begins so clients know the
    // compositor is actively dispatching — not just listening.
    std::cout << "MANSION_SOCKET=" << socket_name << std::endl;
    std::cerr << "Mansion Desktop running on socket: " << socket_name
              << (opts.headless ? " (headless)" : "")
              << (opts.screenshot.empty() ? "" : " [screenshot: " + opts.screenshot + "]")
              << std::endl;

    while (running) {
        if (opts.exit_after_ms >= 0) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - start).count();
            if (elapsed >= opts.exit_after_ms) break;
        }

        auto frame_start = clock::now();

        // Process Wayland client events (windowed mode only).
        if (!opts.headless && display) {
            if (input_process_wayland_client(display)) {
                // xdg_toplevel.close received; exit gracefully.
                running = 0;
                break;
            }
        }

        // Dispatch Wayland events with a short timeout so rendering runs at roughly 60 Hz.
        if (wl_event_loop_dispatch(event_loop, 16) < 0) {
            std::cerr << "Event loop error: " << strerror(errno) << std::endl;
            status = 1;
            break;
        }
        wl_display_flush_clients(wl_display);

        if (g_client_display && wl_display_get_error(g_client_display)) {
            std::cerr << "Host Wayland connection failed: "
                      << strerror(wl_display_get_error(g_client_display)) << std::endl;
            status = 1;
            break;
        }

        if (input_started) input_process();

        /* Execute next script command, if any. */
        if (script) {
            int rc = input_script_step(script, seat, compositor, display);
            if (rc == 1) {
                /* Script requested quit. */
                running = 0;
            }
            if (rc >= 0 && script->remaining_wait_ms > 0) {
                long ms = script->remaining_wait_ms > 16 ? 16 : script->remaining_wait_ms;
                struct timespec ts = {ms / 1000, (ms % 1000) * 1000000L};
                nanosleep(&ts, nullptr);
                script->remaining_wait_ms -= ms;
            }
            if (rc < 0 || rc == 1) {
                input_script_destroy(script);
                script = nullptr;
            }
        }

        /* P2-T06: apply accumulated camera movement / mouse look. */
        if (script && display) {
            auto now = clock::now();
            double delta = std::chrono::duration<double, std::milli>(now - frame_start).count();
            input_script_apply_movement(script, display, delta);
            /* P4-T01: log camera position for collision test verification. */
            log_camera_position(display);
        }

        render(display);
        frame_count++;

        /* P2-T07: frame-rate cap at ~60 Hz to avoid burning CPU and
         * reducing Xwayland flickering. */
        auto frame_end = clock::now();
        double elapsed = std::chrono::duration<double, std::milli>(frame_end - frame_start).count();
        double target = 16.67;  /* 60 fps */
        if (elapsed < target) {
            struct timespec ts = {(long)((target - elapsed) / 1000),
                                  (long)((target - elapsed) * 1000000.0)};
            nanosleep(&ts, nullptr);
        }
    }

    input_script_destroy(script);
    script = nullptr;

    /* Take a screenshot if requested. */
    if (!opts.screenshot.empty()) {
        take_screenshot(display, opts.screenshot.c_str());
        /* Give clients a few seconds to finish their event loops and exit
         * cleanly before we destroy the display socket. */
        auto wait_end = clock::now() + std::chrono::milliseconds(2000);
        while (running && clock::now() < wait_end) {
            if (wl_event_loop_dispatch(event_loop, 0) < 0) break;
            wl_display_flush_clients(wl_display);
        }
    }

    std::cerr << "Mansion Desktop exiting (frames: " << frame_count << ")" << std::endl;
    cleanup();
    return status;
}
