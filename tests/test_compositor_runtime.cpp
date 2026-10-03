#include "compositor-runtime.h"
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>

static void require(bool condition, const char* message) {
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
static void mark(const std::string& path) {
    int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_CLOEXEC, 0600);
    require(fd >= 0, "write fixture barrier"); close(fd);
}
int main(int argc, char** argv) {
    require(argc == 2, "fixture executable argument required");
    alarm(30);
    char temp[] = "/tmp/mansion-runtime-test-XXXXXX";
    require(mkdtemp(temp), "test directory");
    mansion::CompositorRuntime runtime;
    require(!runtime.pump(), "pump stopped server must fail");
    require(!runtime.start("relative"), "relative directory must fail");
    require(!runtime.start(std::string(temp) + "/missing"), "missing directory must fail");
    for (int trial = 0; trial < 16; ++trial) {
        require(runtime.start(temp), runtime.last_error().c_str());
        require(!runtime.start(temp), "double start must fail without losing live server");
        const auto socket = runtime.socket_path();
        const auto directory = socket.substr(0, socket.rfind('/'));
        const std::string marker = std::string(temp) + "/client-" + std::to_string(trial);
        const char* modes[] = {"destroy", "disconnect", "server-stop", "stale-ack", "unknown-ack", "unconfigured", "roleless", "bad-geometry"};
        const char* mode = modes[trial % 8];
        pid_t pid = fork();
        require(pid >= 0, "fork fixture");
        if (!pid) {
            execl(argv[1], argv[1], socket.c_str(), marker.c_str(), mode, nullptr);
            _exit(127);
        }
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (access((marker + ".ready").c_str(), F_OK) < 0) {
            require(runtime.pump(), runtime.last_error().c_str());
            require(std::chrono::steady_clock::now() < deadline, "fixture readiness timed out");
            usleep(1000);
        }
        require(runtime.client_count() == 1, "connected client count");
        require(runtime.surface_count() == 16, "client surface requests reached server");
        require(runtime.toplevel_count() == 16, "registered toplevel count");
        if (trial % 8 == 2) require(runtime.stop(), "shutdown while client connected");
        mark(marker + ".go");
        if (trial % 8 == 0) {
            while (access((marker + ".partial").c_str(), F_OK) < 0) {
                require(runtime.pump(), "partial destruction pump");
                require(std::chrono::steady_clock::now() < deadline, "partial destruction timeout");
                usleep(1000);
            }
            require(runtime.toplevel_count() == 8 && runtime.surface_count() == 8, "non-first window removal");
            mark(marker + ".resume");
        }
        int status = 0;
        pid_t waited;
        while ((waited = waitpid(pid, &status, WNOHANG)) == 0) {
            if (runtime.running()) require(runtime.pump(), runtime.last_error().c_str());
            require(std::chrono::steady_clock::now() < deadline, "fixture completion timed out");
            usleep(1000);
        }
        require(waited == pid, "could not collect fixture exit status");
        require(WIFEXITED(status) && WEXITSTATUS(status) == 0, "fixture returned failure");
        if (runtime.running()) {
            for (int i = 0; i < 3; ++i) require(runtime.pump(), "disconnect pump");
            require(runtime.toplevel_count() == 0, "destroy/disconnect leaked toplevels");
            require(runtime.surface_count() == 0, "destroy/disconnect leaked surfaces");
            require(runtime.client_count() == 0, "disconnect leaked client");
        }
        require(runtime.stop() && runtime.stop(), "idempotent stop");
        require(access(socket.c_str(), F_OK) < 0 && access(directory.c_str(), F_OK) < 0, "socket/directory cleanup");
        for (const char* suffix : {".ready", ".go", ".partial", ".resume", ".ok"}) unlink((marker + suffix).c_str());
    }
    std::string scoped_socket;
    {
        mansion::CompositorRuntime scoped;
        require(scoped.start(temp), "scoped server start");
        scoped_socket = scoped.socket_path();
    }
    require(access(scoped_socket.c_str(), F_OK) < 0, "destructor did not remove socket");
    require(rmdir(temp) == 0, "test left files");
    std::puts("RUNTIME_LIFECYCLE_OK: 16 real client trials; xdg handshake, seat v1/v4, lifecycle and protocol errors");
}
