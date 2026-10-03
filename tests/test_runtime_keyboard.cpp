#include "compositor-runtime.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iterator>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

static void check(bool ok, const char* message) {
    if (!ok) { std::fprintf(stderr, "KEYBOARD FAIL: %s\n", message); std::exit(1); }
}
static std::string read(const std::string& path) {
    std::ifstream stream(path); return {std::istreambuf_iterator<char>(stream), {}};
}
static void mark(const std::string& path) { std::ofstream stream(path); check(bool(stream), "marker creation"); }
static void until(mansion::CompositorRuntime& runtime, const std::function<bool()>& predicate) {
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!predicate()) {
        check(runtime.pump(), runtime.last_error().c_str());
        check(std::chrono::steady_clock::now() < deadline, "timed out waiting for fixture");
        usleep(1000);
    }
}
static void wait_file(mansion::CompositorRuntime& runtime, const std::string& path) {
    until(runtime, [&] { return access(path.c_str(), F_OK) == 0; });
}
static void wait_event(mansion::CompositorRuntime& runtime, const std::string& path, const std::string& event) {
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
    char directory[] = "/tmp/mansion-keyboard-XXXXXX"; check(mkdtemp(directory), "test directory");
    mansion::CompositorRuntime runtime;
    check(!runtime.focus_keyboard(0) && !runtime.keyboard_key(30, true), "stopped input rejected");
    check(runtime.start(directory), runtime.last_error().c_str());
    const std::string a = std::string(directory) + "/a", b = std::string(directory) + "/b";
    const auto pid_a = spawn(argv[1], runtime.socket_path(), a);
    wait_file(runtime, a + ".ready");
    const auto windows_a = runtime.toplevel_handles(); check(windows_a.size() == 2, "two same-client windows");
    auto surfaces = runtime.surface_handles(); check(surfaces.size() == 3, "roleless surface fixture");
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
    check(runtime.keyboard_focus_handle() == 0 && !runtime.keyboard_key(30, true), "world mode sends no keys");
    check(!runtime.focus_keyboard(roleless) && !runtime.focus_keyboard(-1), "invalid focus rejected");
    check(runtime.focus_keyboard(windows_a[0]), "focus first client");
    wait_event(runtime, a + ".events", "1 enter 0\n1 mods 0 0 0 0\n");
    check(read(b + ".events").empty(), "focus leaked to other client");
    check(!runtime.keyboard_key(0, true) && !runtime.keyboard_key(768, true), "invalid evdev codes rejected");
    check(runtime.keyboard_key(42, true) && runtime.keyboard_key(30, true) &&
          runtime.keyboard_key(30, true) && runtime.keyboard_key(30, false) && runtime.keyboard_key(42, false), "Shift+A sequence");
    wait_event(runtime, a + ".events", "1 key 42 0");
    auto events = read(a + ".events");
    check(occurrences(events, "1 key 30 1 65\n") == 1 && events.find("1 key 30 0 65\n") != std::string::npos,
          "US uppercase, key release and duplicate/echo suppression");
    check(runtime.keyboard_key(29, true) && runtime.keyboard_key(56, true) && runtime.keyboard_key(30, true), "held chord");
    check(runtime.focus_keyboard(windows_b[0]), "switch client");
    wait_event(runtime, a + ".events", "1 leave\n");
    events = read(a + ".events");
    check(events.find("1 key 29 0") != std::string::npos && events.find("1 key 56 0") != std::string::npos &&
          events.ends_with("1 mods 0 0 0 0\n1 leave\n"), "release held keys/modifiers before leave");
    wait_event(runtime, b + ".events", "1 enter 0\n1 mods 0 0 0 0\n");
    check(runtime.keyboard_key(30, true) && runtime.keyboard_key(30, false), "second client key");
    wait_event(runtime, b + ".events", "1 key 30 0 97\n");
    check(read(a + ".events") == events, "keys leaked to unfocused client");
    check(!runtime.focus_keyboard(roleless) && runtime.keyboard_focus_handle() == windows_b[0], "invalid focus preserves current target");
    check(runtime.keyboard_key(58, true) && runtime.keyboard_key(58, false), "Caps Lock toggle");
    wait_event(runtime, b + ".events", "1 mods 0 0 2 0\n");
    check(runtime.focus_keyboard(windows_b[1]) && runtime.focus_keyboard(windows_b[1]), "same-client switch/idempotence");
    check(runtime.keyboard_key(42, true), "hold Shift before late bind");
    mark(b + ".late"); wait_file(runtime, b + ".late-ok");
    events = read(b + ".events");
    check(occurrences(events, "1 enter 0\n") == 2, "same-focus request produced extra enter");
    check(events.find("2 enter 1\n2 held 42\n2 mods 1 0 0 0\n") != std::string::npos, "late keyboard missing held keys/modifiers or stale Caps Lock");
    mark(b + ".detach"); wait_file(runtime, b + ".detach-ok");
    check(runtime.keyboard_focus_handle() == 0 && !runtime.focus_keyboard(windows_b[1]), "detached target retained focus");
    wait_event(runtime, b + ".events", "2 leave\n");
    check(read(b + ".events").find("2 key 42 0") != std::string::npos, "detach did not release modifier");
    check(runtime.focus_keyboard(windows_a[0]) && runtime.keyboard_key(42, true) && runtime.keyboard_key(30, true), "focus before destruction");
    mark(a + ".destroy"); wait_file(runtime, a + ".destroy-ok");
    check(runtime.keyboard_focus_handle() == 0 && !runtime.keyboard_key(30, false), "destroyed focus remained live");
    check(runtime.focus_keyboard(0), "clear focus idempotently");
    mark(a + ".finish"); mark(b + ".finish");
    wait_file(runtime, a + ".ok"); wait_file(runtime, b + ".ok");
    for (auto pid : {pid_a, pid_b}) {
        int status = 0; check(waitpid(pid, &status, 0) == pid && WIFEXITED(status) && WEXITSTATUS(status) == 0, "fixture exit");
    }
    check(runtime.stop() && runtime.start(directory), "restart");
    check(!runtime.focus_keyboard(windows_a[0]) && runtime.keyboard_focus_handle() == 0, "stale focus after restart");
    check(runtime.stop(), "final stop");
    for (const auto& base : {a, b}) for (const char* suffix : {"events", "ready", "late", "late-ok", "detach", "detach-ok", "destroy", "destroy-ok", "finish", "ok"})
        unlink((base + "." + suffix).c_str());
    check(rmdir(directory) == 0, "test cleanup");
    std::puts("RUNTIME_KEYBOARD_OK: two clients, focus/key/modifier routing, late bind, detach and destruction");
}
