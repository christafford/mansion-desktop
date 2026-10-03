#include "compositor-runtime.h"
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

static void check(bool ok, const char* message) {
    if (!ok) { std::fprintf(stderr, "FRAME FAIL: %s\n", message); std::exit(1); }
}
static const std::vector<uint8_t> argb = {255,0,0,255, 0,255,0,255, 0,0,255,255, 128,64,32,128,
                                         0,0,0,0, 255,255,255,255, 18,52,86,255, 171,205,239,255};
static const std::vector<uint8_t> xrgb = {10,20,30,255, 40,50,60,255, 70,80,90,255,
                                         100,110,120,255, 130,140,150,255, 160,170,180,255};
static void wait_marker(mansion::CompositorRuntime& runtime, const std::string& path) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (access(path.c_str(), F_OK)) {
        check(runtime.pump(), runtime.last_error().c_str());
        check(std::chrono::steady_clock::now() < deadline, path.c_str());
        usleep(1000);
    }
}
static void mark(const std::string& path) {
    int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_CLOEXEC, 0600);
    check(fd >= 0, "barrier"); close(fd);
}
static void verify(const mansion::OwnedFrame& f, const std::string& stage) {
    check(bool(f), "frame exists");
    const bool is_xrgb = stage == "replacement" || stage == "pending";
    const bool detached = stage == "detached";
    const bool scaled = stage == "scaled" || stage == "pending-destroyed" || detached;
    const int transform = stage.starts_with("transform-") ? stage.back() - '0' : scaled ? 1 : 0;
    const uint64_t revision = stage == "initial" || stage == "destroyed" ? 1 : is_xrgb ? 2 :
        scaled ? (detached ? 4 : 3) : stage == "remapped" ? 6 : 6 + transform;
    check(f->revision == revision, "revision changes only on content or committed metadata");
    check(f->mapped == !detached, "mapped state");
    check(f->width == (detached ? 0 : is_xrgb ? 2 : 4) && f->height == (detached ? 0 : is_xrgb ? 3 : 2), "buffer dimensions");
    check(f->stride == f->width * 4, "packed output stride");
    check(f->transform == transform && f->scale == (scaled ? 2 : 1), "committed transform/scale");
    check(f->logical_width == ((transform & 1) ? f->height : f->width) / f->scale &&
          f->logical_height == ((transform & 1) ? f->width : f->height) / f->scale, "logical dimensions");
    check(f->pixels == (detached ? std::vector<uint8_t>{} : is_xrgb ? xrgb : argb), "exact RGBA, alpha, row order, padding removed");
    if (!detached) check(f->source_stride == (is_xrgb ? 16 : 24) && f->source_format == (is_xrgb ? 1u : 0u), "source metadata");
}
int main(int argc, char** argv) {
    check(argc == 2, "fixture executable"); alarm(45);
    char directory[] = "/tmp/mansion-frame-test-XXXXXX"; check(mkdtemp(directory), "temp directory");
    mansion::CompositorRuntime runtime;
    int64_t previous_handle = 0;
    mansion::OwnedFrame retained;
    const std::vector<std::string> stages = {"initial", "destroyed", "replacement", "pending", "scaled",
        "pending-destroyed", "detached", "remapped", "transform-0", "transform-1", "transform-2", "transform-3",
        "transform-4", "transform-5", "transform-6", "transform-7"};
    const char* modes[] = {"frames", "frames", "scale", "transform", "oversize", "format", "divisibility", "truncated"};
    for (int trial = 0; trial < 8; ++trial) {
        check(runtime.start(directory), runtime.last_error().c_str());
        check(!runtime.snapshot(previous_handle), "stale handle invalid after restart");
        const std::string marker = std::string(directory) + "/trial-" + std::to_string(trial);
        pid_t pid = fork(); check(pid >= 0, "fork");
        if (!pid) { execl(argv[1], argv[1], runtime.socket_path().c_str(), marker.c_str(), modes[trial], nullptr); _exit(127); }
        if (trial < 2) {
            for (const auto& stage : stages) {
                wait_marker(runtime, marker + "." + stage + ".ready");
                auto handles = runtime.surface_handles(); check(handles.size() == 1, "exactly one surface");
                auto frame = runtime.snapshot(handles.front()); verify(frame, stage);
                check(frame->handle == handles.front(), "snapshot handle matches enumeration");
                if (stage == "initial") {
                    check(frame->handle != previous_handle, "handle cannot alias an earlier session");
                    previous_handle = frame->handle;
                    if (!retained) retained = frame;
                }
                check(retained->pixels == argb && retained->revision == 1, "retained snapshot remains immutable");
                mark(marker + "." + stage + ".go");
            }
        }
        wait_marker(runtime, marker + ".ok");
        int status; check(waitpid(pid, &status, 0) == pid && WIFEXITED(status) && WEXITSTATUS(status) == 0, "client completed");
        for (int i = 0; i < 3; ++i) check(runtime.pump(), "cleanup pump");
        check(runtime.surface_count() == 0 && runtime.surface_handles().empty(), "disconnect clears surfaces");
        check(!runtime.snapshot(previous_handle), "destroyed handle invalid");
        check(retained->pixels == argb, "owned frame survives disconnect");
        check(runtime.stop(), "stop");
        for (auto& stage : stages) for (const char* suffix : {".ready", ".go"}) unlink((marker + "." + stage + suffix).c_str());
        unlink((marker + ".ok").c_str());
    }
    check(rmdir(directory) == 0, "all files removed");
    std::puts("FRAME_SNAPSHOTS_OK: two sessions, 32 pixel/state barriers and six rejected buffer/state cases");
}
