#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>

#include "launch.h"

struct MansionApp {
    struct wl_display* display;
    pid_t pid;
    bool has_pid;
    char* name;
    struct wl_list link;
};

void destroy_app(struct MansionApp* app) {
    if (!app) return;

    // Reap child process if still running
    if (app->has_pid && app->pid > 0) {
        int status;
        pid_t result = waitpid(app->pid, &status, WNOHANG);
        if (result == 0) {
            // Process still running, send SIGTERM
            kill(app->pid, SIGTERM);
            // Wait briefly for graceful shutdown
            usleep(100000);  // 100ms
            result = waitpid(app->pid, &status, WNOHANG);
            if (result == 0) {
                // Force kill
                kill(app->pid, SIGKILL);
                waitpid(app->pid, &status, 0);
            }
        }
    }

    free(app->name);
    delete app;
}

MansionApp* launch_app(struct wl_display* display, const char* executable, const char* name) {
    auto* app = new MansionApp;
    app->display = display;
    app->name = strdup(name);
    app->has_pid = false;
    app->pid = -1;
    wl_list_init(&app->link);

    // Set up environment
    const char* wayland_display = getenv("WAYLAND_DISPLAY");
    std::string display_env = std::string("WAYLAND_DISPLAY=") +
        (wayland_display ? wayland_display : "mansion-desktop");

    std::string xdg_runtime_dir = std::string("/tmp/mansion-") + std::to_string(getpid());

    if (mkdir(xdg_runtime_dir.c_str(), 0700) < 0) {
        std::cerr << "Failed to create runtime directory: " << xdg_runtime_dir << std::endl;
        free(app->name);
        delete app;
        return nullptr;
    }

    // Build environment for child
    const char* env_path = getenv("PATH");
    std::string path_env = std::string("PATH=") + (env_path ? env_path : "/usr/bin:/bin");

    char* new_env[] = {
        const_cast<char*>(display_env.c_str()),
        const_cast<char*>(path_env.c_str()),
        const_cast<char*>((std::string("XDG_RUNTIME_DIR=") + xdg_runtime_dir).c_str()),
        nullptr,
    };

    pid_t child_pid = fork();
    if (child_pid < 0) {
        std::cerr << "Failed to fork" << std::endl;
        rmdir(xdg_runtime_dir.c_str());
        free(app->name);
        delete app;
        return nullptr;
    }

    if (child_pid == 0) {
        // Child process
        // Reset signal handlers to defaults
        signal(SIGCHLD, SIG_DFL);

        // Set environment
        for (int i = 0; new_env[i]; i++) {
            putenv(new_env[i]);
        }

        // Execute the application
        execvp(executable, const_cast<char*const*>(new_env));

        // If we get here, exec failed
        std::cerr << "Failed to exec " << executable << ": " << strerror(errno) << std::endl;
        rmdir(xdg_runtime_dir.c_str());
        _exit(1);
    }

    // Parent process
    app->pid = child_pid;
    app->has_pid = true;

    return app;
}

pid_t app_pid(struct MansionApp* app) {
    return app ? app->pid : -1;
}

bool app_has_pid(struct MansionApp* app) {
    return app && app->has_pid;
}
