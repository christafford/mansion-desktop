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
    if (!ok) { std::fprintf(stderr, "RESIZE FAIL: %s\n", message); std::exit(1); }
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
static pid_t spawn(const char* fixture, const std::string& socket, const std::string& base) {
    pid_t child = fork(); check(child >= 0, "fork");
    if (!child) { execl(fixture, fixture, socket.c_str(), base.c_str(), nullptr); _exit(127); }
    return child;
}
static void step(elsewhere::CompositorRuntime& r, const std::string& base, int n) {
    mark(base + "." + std::to_string(n)); wait_file(r, base + "." + std::to_string(n) + "-ok");
}
int main(int argc,char** argv) {
    check(argc==2,"fixture executable");alarm(30);
    char directory[]="/tmp/elsewhere-resize-XXXXXX";check(mkdtemp(directory),"directory");
    elsewhere::CompositorRuntime r;
    check(!r.request_resize(1,100,100) && !r.window_state(1),"stopped server");
    check(r.start(directory),"start");
    const std::string a=std::string(directory)+"/a",b=std::string(directory)+"/b";
    auto pid=spawn(argv[1],r.socket_path(),a);wait_file(r,a+".ready");
    auto h=r.toplevel_handles()[0]; auto old=r.snapshot(h);auto state=*r.window_state(h);
    check(state.x==4 && state.y==6 && state.width==72 && state.height==48 && state.min_width==32 && state.max_width==120,"initial committed geometry/limits");
    check(state.sent_serial && state.committed_serial==state.acked_serial,"initial ack commit");
    auto other=spawn(argv[1],r.socket_path(),b);wait_file(r,b+".ready");const auto other_events=read(b+".events");
    check(!r.request_resize(-1,50,50) && !r.request_resize(h,0,30) && !r.request_resize(h,2049,30),"invalid request bounds/handle");
    step(r,a,1);check(r.window_state(h)->x==4 && r.window_state(h)->min_width==32,"pending metadata exposed before commit");
    check(r.request_resize(h,10,10),"minimum request");
    wait_event(r,a+".events"," 32 24\n");auto serial=r.window_state(h)->sent_serial;
    check(r.snapshot(h)==old && r.window_state(h)->committed_serial==state.committed_serial,"configure replaced old content/state");
    step(r,a,2);check(r.window_state(h)->acked_serial==serial && r.window_state(h)->committed_serial==state.committed_serial && r.snapshot(h)==old,"ack prematurely applied content");
    step(r,a,3);auto chosen=r.snapshot(h);state=*r.window_state(h);
    check(chosen->width==50 && chosen->height==40 && chosen->pixels[0]==0x22 && old->width==80 && old->pixels[0]==0x11,"client choice or owned old pixels lost");
    check(state.committed_serial==serial && state.x==2 && state.y==3 && state.width==44 && state.height==30 && state.min_width==40 && state.max_width==110,"committed geometry/limits not applied atomically");
    check(r.request_resize(h,1000,1000),"maximum request");wait_event(r,a+".events"," 110 90\n");
    step(r,a,4);check(r.snapshot(h)==chosen && r.window_state(h)->committed_serial==r.window_state(h)->sent_serial,"decline lost pixels or commit acknowledgement");
    auto sent=r.window_state(h)->sent_serial;check(r.request_resize(h,1000,1000) && r.window_state(h)->sent_serial==sent,"declined request resend loop");
    check(r.request_resize(h,70,60) && r.request_resize(h,90,70),"successive requests");wait_event(r,a+".events"," 90 70\n");
    step(r,a,5);check(r.snapshot(h)->width==110 && r.window_state(h)->width==100 && r.window_state(h)->committed_serial==r.window_state(h)->sent_serial,"coalesced ack/commit");
    check(read(b+".events")==other_events,"configure leaked to another client");
    auto pixels=r.snapshot(h);step(r,a,6);check(r.snapshot(h)==pixels && r.window_state(h)->x==0 && r.window_state(h)->y==0 && r.window_state(h)->width==110 && r.window_state(h)->height==84,"geometry-only clamping");
    step(r,a,7);check(r.snapshot(h)->width==120 && r.window_state(h)->width==110,"explicit effective geometry incorrectly recalculated");
    for(int i=0;i<64;++i)check(r.request_resize(h,40+i,40+i%40),"bounded pending requests");
    check(!r.request_resize(h,109,89),"unbounded configure queue");
    step(r,a,8);check(!r.window_state(h) && !r.request_resize(h,80,60),"destroy pending target");
    mark(a+".finish");mark(b+".finish");wait_file(r,a+".ok");wait_file(r,b+".ok");
    for(auto child:{pid,other}){int status=0;check(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status),"client exit");}
    until(r,[&]{return r.client_count()==0;});
    // Invalid acknowledgement and limit requests must disconnect only the offender.
    for(const char* mode:{"stale","negative","conflict","outside"}) {
        const auto base=std::string(directory)+"/"+mode;auto child=spawn(argv[1],r.socket_path(),base);wait_file(r,base+".ready");
        auto target=r.toplevel_handles()[0];
        if(std::string(mode)=="stale") {
            check(r.request_resize(target,50,40),"first serial");wait_event(r,base+".events"," 50 40\n");
            step(r,base,1);step(r,base,2);step(r,base,3);
            check(r.request_resize(target,60,50),"second serial");wait_event(r,base+".events"," 60 50\n");step(r,base,4);
        }
        mark(base+"."+mode);until(r,[&]{return r.client_count()==0;});
        int status=0;check(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status),"negative fixture exit");
    }
    check(r.stop() && r.start(directory) && !r.request_resize(h,60,50) && r.stop(),"restart invalidates resize handle");
    for(const char* name:{"a","b","stale","negative","conflict","outside"}) {
        auto base=std::string(directory)+"/"+name;
        for(const char* suffix:{"events","ready","finish","ok","stale","negative","conflict","outside"})unlink((base+"."+suffix).c_str());
        for(int i=1;i<=8;++i){unlink((base+"."+std::to_string(i)).c_str());unlink((base+"."+std::to_string(i)+"-ok").c_str());}
    }
    check(rmdir(directory)==0,"cleanup");
    puts("RUNTIME_RESIZE_OK: configure/ack/commit, client choice, constraints, geometry, isolation, queue bound, destruction and invalid requests");
}
