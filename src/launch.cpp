#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

#include "launch.h"

struct MansionApp {
    struct wl_display* display;
    pid_t pid;
    std::string command;
};

void destroy_app(struct MansionApp* app) {
    if (!app) return;
    if (app->pid > 0) {
        int status = 0;
        pid_t result = waitpid(app->pid, &status, WNOHANG);
        if (result == 0) {
            kill(app->pid, SIGTERM);
            // Give the client a moment to disconnect cleanly before forcing it.
            for (int i = 0; i < 10 && result == 0; i++) {
                usleep(100000);
                result = waitpid(app->pid, &status, WNOHANG);
            }
            if (result == 0) {
                kill(app->pid, SIGKILL);
                waitpid(app->pid, &status, 0);
            }
        }
    }
    delete app;
}

// Whitespace split; quoting is not interpreted (see docs/TASKS.md P1-T08).
static std::vector<std::string> split_command(const char* command) {
    std::vector<std::string> words;
    std::istringstream in(command);
    std::string word;
    while (in >> word) words.push_back(word);
    return words;
}

MansionApp* launch_app(struct wl_display* display, const char* socket_name, const char* command) {
    std::vector<std::string> words = split_command(command);
    if (words.empty()) {
        std::cerr << "launch: empty command" << std::endl;
        return nullptr;
    }

    pid_t child_pid = fork();
    if (child_pid < 0) {
        std::cerr << "launch: fork failed: " << strerror(errno) << std::endl;
        return nullptr;
    }

    if (child_pid == 0) {
        // Child. The runtime directory is inherited so the client finds our socket there.
        signal(SIGINT, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        signal(SIGPIPE, SIG_DFL);
        setenv("WAYLAND_DISPLAY", socket_name, 1);
        // Do not let toolkits fall back to the host X server and open a window on the host desktop.
        unsetenv("DISPLAY");
        setenv("GDK_BACKEND", "wayland", 1);
        setenv("QT_QPA_PLATFORM", "wayland", 1);

        std::vector<char*> argv;
        argv.reserve(words.size() + 1);
        for (auto& w : words) argv.push_back(const_cast<char*>(w.c_str()));
        argv.push_back(nullptr);
        execvp(argv[0], argv.data());
        std::cerr << "launch: exec " << words[0] << " failed: " << strerror(errno) << std::endl;
        _exit(127);
    }

    auto* app = new MansionApp;
    app->display = display;
    app->pid = child_pid;
    app->command = command;
    return app;
}

pid_t app_pid(struct MansionApp* app) {
    return app ? app->pid : -1;
}

bool app_has_pid(struct MansionApp* app) {
    return app && app->pid > 0;
}
