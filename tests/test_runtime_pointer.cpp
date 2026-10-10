#include "compositor-runtime.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iterator>
#include <limits>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

static void check(bool ok, const char* message) {
    if (!ok) { std::fprintf(stderr, "POINTER FAIL: %s\n", message); std::exit(1); }
}
static std::string read(const std::string& path) {
    std::ifstream stream(path); return {std::istreambuf_iterator<char>(stream), {}};
}
static void mark(const std::string& path) { std::ofstream stream(path); check(bool(stream), "marker creation"); }
static void until(elsewhere::CompositorRuntime& runtime, const std::function<bool()>& predicate) {
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!predicate()) {
        check(runtime.pump(), runtime.last_error().c_str());
        check(std::chrono::steady_clock::now() < deadline, "timed out waiting for fixture");
        usleep(1000);
    }
}
static void wait_file(elsewhere::CompositorRuntime& runtime, const std::string& path) {
    until(runtime, [&] { return access(path.c_str(), F_OK) == 0; });
}
static void wait_event(elsewhere::CompositorRuntime& runtime, const std::string& path, const std::string& event) {
    until(runtime, [&] { return read(path).find(event) != std::string::npos; });
}
static size_t occurrences(const std::string& text, const std::string& part) {
    size_t count = 0, pos = 0;
    while ((pos = text.find(part, pos)) != std::string::npos) { ++count; pos += part.size(); }
    return count;
}
static pid_t spawn(const char* fixture, const std::string& socket, const std::string& base) {
    pid_t child = fork(); check(child >= 0, "fork");
    if (!child) { execl(fixture, fixture, socket.c_str(), base.c_str(), nullptr); _exit(127); }
    return child;
}
int main(int argc, char** argv) {
    check(argc == 2, "fixture executable"); alarm(30);
    char directory[] = "/tmp/elsewhere-pointer-XXXXXX"; check(mkdtemp(directory), "test directory");
    elsewhere::CompositorRuntime runtime;
    check(!runtime.pointer_motion(0, 0, 0) && !runtime.pointer_button(272, true) && !runtime.pointer_axis(0, 10), "stopped input rejected");
    runtime.pointer_reset();
    check(runtime.start(directory), runtime.last_error().c_str());
    const std::string a = std::string(directory) + "/a", b = std::string(directory) + "/b", c = std::string(directory) + "/c";
    const auto pid_a = spawn(argv[1], runtime.socket_path(), a);
    wait_file(runtime, a + ".ready");
    const auto windows_a = runtime.toplevel_handles(); check(windows_a.size() == 2, "first client windows");
    const auto surfaces = runtime.surface_handles();
    const auto roleless_it = std::find_if(surfaces.begin(), surfaces.end(), [&](auto h) {
        return std::find(windows_a.begin(), windows_a.end(), h) == windows_a.end();
    });
    check(roleless_it != surfaces.end(), "roleless handle exists");
    const auto roleless = *roleless_it;
    const auto pid_b = spawn(argv[1], runtime.socket_path(), b);
    wait_file(runtime, b + ".ready");
    auto windows_b = runtime.toplevel_handles();
    std::erase_if(windows_b, [&](auto h) { return std::find(windows_a.begin(), windows_a.end(), h) != windows_a.end(); });
    check(windows_b.size() == 2, "second client windows");
    check(!runtime.pointer_button(272, true) && !runtime.pointer_axis(10, 0), "world input rejected");
    check(!runtime.pointer_motion(roleless, 0, 0) && !runtime.pointer_motion(-1, 0, 0), "invalid targets");
    check(!runtime.pointer_motion(windows_a[0], -1, 0) && !runtime.pointer_motion(windows_a[0], 2, 0), "outside ungrabbed target");
    check(!runtime.pointer_motion(windows_a[0], std::numeric_limits<double>::quiet_NaN(), 0), "NaN rejected");
    check(runtime.pointer_motion(windows_a[0], .25, 1.5), "enter first client");
    wait_event(runtime, a + ".events", "1 enter 0.250 1.500\n");
    check(read(b + ".events").empty(), "enter leaked to other client");
    check(runtime.pointer_motion(windows_a[0], 1.5, .25), "motion");
    check(runtime.pointer_axis(-2.5, 10), "two scroll axes");
    check(!runtime.pointer_axis(0, std::numeric_limits<double>::infinity()), "infinite axis rejected");
    check(!runtime.pointer_button(271, true) && !runtime.pointer_button(280, true), "invalid button rejected");
    check(runtime.pointer_button(272, true) && runtime.pointer_button(272, true) && runtime.pointer_button(274, true), "multiple held buttons");
    check(runtime.pointer_grabbed(), "missing implicit grab");
    check(!runtime.pointer_motion(windows_b[0], 0, 0) && !runtime.pointer_motion(0, 0, 0), "grab cannot switch clients or leave");
    check(runtime.pointer_motion(windows_a[0], -3.5, 5.25), "drag outside target");
    check(runtime.pointer_button(272, false) && runtime.pointer_grabbed(), "other button retains grab");
    wait_event(runtime, a + ".events", "1 button 272 0\n");
    auto events = read(a + ".events");
    check(occurrences(events, "1 button 272 1\n") == 1, "duplicate press delivered");
    check(events.find("1 motion 1.500 0.250\n1 axis 1 -2.500\n1 axis 0 10.000\n") != std::string::npos, "motion/axis coordinates/order");
    check(events.find("1 motion -3.500 5.250\n") != std::string::npos, "outside grab coordinates");
    check(read(b + ".events").empty(), "grab events leaked to other client");
    runtime.pointer_reset();
    wait_event(runtime, a + ".events", "1 button 274 0\n1 leave\n");
    check(!runtime.pointer_grabbed() && runtime.pointer_focus_handle() == 0, "reset retained state");
    events = read(a + ".events");
    check(runtime.pointer_motion(windows_b[0], 1, 1), "switch client");
    for (uint32_t code : {272u, 273u, 274u, 275u, 276u})
        check(runtime.pointer_button(code, true) && runtime.pointer_button(code, false), "frontend button codes");
    wait_event(runtime, b + ".events", "1 button 276 0\n");
    for (uint32_t code : {272u, 273u, 274u, 275u, 276u})
        check(read(b + ".events").find("1 button " + std::to_string(code) + " 1\n1 button " + std::to_string(code) + " 0\n") != std::string::npos, "button mapping on wire");
    check(read(a + ".events") == events, "second client events leaked");
    check(runtime.pointer_motion(windows_b[1], .5, .75), "same-client window switch");
    mark(b + ".late"); wait_file(runtime, b + ".late-ok");
    wait_event(runtime, b + ".events", "2 enter 0.500 0.750\n");
    check(runtime.pointer_button(273, true), "hold before detach");
    mark(b + ".detach"); wait_file(runtime, b + ".detach-ok");
    wait_event(runtime, b + ".events", "2 leave\n");
    check(read(b + ".events").find("2 button 273 0\n") != std::string::npos, "detach release");
    check(!runtime.pointer_grabbed() && !runtime.pointer_focus_handle() && !runtime.pointer_motion(windows_b[1], 0, 0), "detach retained input");
    check(runtime.pointer_motion(windows_a[0], 0, 0) && runtime.pointer_button(272, true), "hold before surface destruction");
    mark(a + ".destroy"); wait_file(runtime, a + ".destroy-ok");
    check(!runtime.pointer_grabbed() && !runtime.pointer_focus_handle(), "destroy retained input");
    mark(a + ".finish"); mark(b + ".finish");
    wait_file(runtime, a + ".ok"); wait_file(runtime, b + ".ok");
    for (auto pid : {pid_a, pid_b}) {
        int status = 0; check(waitpid(pid, &status, 0) == pid && WIFEXITED(status) && WEXITSTATUS(status) == 0, "fixture exit");
    }
    until(runtime, [&] { return runtime.client_count() == 0; });
    const auto pid_c = spawn(argv[1], runtime.socket_path(), c);
    wait_file(runtime, c + ".ready");
    check(runtime.pointer_motion(runtime.toplevel_handles()[0], 0, 0) && runtime.pointer_button(272, true), "hold before disconnect");
    check(kill(pid_c, SIGKILL) == 0, "kill owned fixture");
    int status = 0; check(waitpid(pid_c, &status, 0) == pid_c && WIFSIGNALED(status), "killed fixture reaped");
    until(runtime, [&] { return runtime.client_count() == 0; });
    check(!runtime.pointer_grabbed() && !runtime.pointer_focus_handle(), "disconnect retained input");
    check(runtime.stop() && runtime.start(directory), "restart");
    check(!runtime.pointer_motion(windows_a[0], 0, 0), "stale handle after restart");
    check(runtime.stop(), "final stop");
    for (const auto& base : {a, b, c}) for (const char* suffix : {"events", "ready", "late", "late-ok", "detach", "detach-ok", "destroy", "destroy-ok", "finish", "ok"})
        unlink((base + "." + suffix).c_str());
    check(rmdir(directory) == 0, "test cleanup");
    std::puts("RUNTIME_POINTER_OK: client isolation, coordinates, buttons, scroll, grab, late bind, detach, destruction and disconnect");
}
