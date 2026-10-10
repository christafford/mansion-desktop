// Real protocol client and native runtime. No display or GPU is used.
#include "compositor-runtime.h"
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include <sys/mman.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>

static void check(bool ok, const char* why) { if (!ok) { std::fprintf(stderr,"TREE FAIL: %s\n",why); std::exit(1); } }
struct Client {
    wl_display* display{}; wl_compositor* compositor{}; wl_subcompositor* subcompositor{}; wl_shm* shm{};
    xdg_wm_base* shell{}; wl_seat* seat{}; wl_surface* entered{}; double x{},y{}; int buttons{},frames{},pointer_frames{},axis_sources{},axes{};
};
static void global(void* data, wl_registry* registry, uint32_t name, const char* interface, uint32_t) {
    auto& c=*static_cast<Client*>(data);
    if (!std::strcmp(interface,"wl_compositor")) c.compositor=static_cast<wl_compositor*>(wl_registry_bind(registry,name,&wl_compositor_interface,4));
    if (!std::strcmp(interface,"wl_subcompositor")) c.subcompositor=static_cast<wl_subcompositor*>(wl_registry_bind(registry,name,&wl_subcompositor_interface,1));
    if (!std::strcmp(interface,"wl_shm")) c.shm=static_cast<wl_shm*>(wl_registry_bind(registry,name,&wl_shm_interface,1));
    if (!std::strcmp(interface,"xdg_wm_base")) c.shell=static_cast<xdg_wm_base*>(wl_registry_bind(registry,name,&xdg_wm_base_interface,3));
    if (!std::strcmp(interface,"wl_seat")) c.seat=static_cast<wl_seat*>(wl_registry_bind(registry,name,&wl_seat_interface,5));
}
static void removed(void*,wl_registry*,uint32_t) {}
static const wl_registry_listener globals={global,removed};
static void configured(void*,xdg_surface* s,uint32_t serial) { xdg_surface_ack_configure(s,serial); }
static const xdg_surface_listener xdg_listener={configured};
static void top_configure(void*,xdg_toplevel*,int32_t,int32_t,wl_array*) {}
static void closed(void*,xdg_toplevel*) {}
static const xdg_toplevel_listener top_listener=[] { xdg_toplevel_listener l{}; l.configure=top_configure; l.close=closed; return l; }();
static void enter(void* data,wl_pointer*,uint32_t,wl_surface* s,wl_fixed_t x,wl_fixed_t y) { auto& c=*static_cast<Client*>(data); c.entered=s;c.x=wl_fixed_to_double(x);c.y=wl_fixed_to_double(y); }
static void leave(void* data,wl_pointer*,uint32_t,wl_surface*) { static_cast<Client*>(data)->entered=nullptr; }
static void motion(void* data,wl_pointer*,uint32_t,wl_fixed_t x,wl_fixed_t y) { auto& c=*static_cast<Client*>(data);c.x=wl_fixed_to_double(x);c.y=wl_fixed_to_double(y); }
static void button(void* data,wl_pointer*,uint32_t,uint32_t,uint32_t,uint32_t) { ++static_cast<Client*>(data)->buttons; }
static void axis(void* data,wl_pointer*,uint32_t,uint32_t,wl_fixed_t) { ++static_cast<Client*>(data)->axes; }
static void pointer_frame(void* data,wl_pointer*) { ++static_cast<Client*>(data)->pointer_frames; }
static void axis_source(void* data,wl_pointer*,uint32_t source) { check(source==WL_POINTER_AXIS_SOURCE_WHEEL,"wheel axis source");++static_cast<Client*>(data)->axis_sources; }
static const wl_pointer_listener pointer_listener=[] { wl_pointer_listener l{}; l.enter=enter; l.leave=leave; l.motion=motion; l.button=button; l.axis=axis; l.frame=pointer_frame; l.axis_source=axis_source; return l; }();
static void frame(void* data,wl_callback* cb,uint32_t) { ++static_cast<Client*>(data)->frames;wl_callback_destroy(cb); }
static const wl_callback_listener frame_listener={frame};
static void roundtrip(Client& c) { check(wl_display_roundtrip(c.display)>=0 && wl_display_roundtrip(c.display)>=0,"roundtrip"); }
static void fill(Client& c,wl_surface* s,uint32_t color,int width=2,int height=2,bool split=false) {
    int fd=memfd_create("tree-pixels",MFD_CLOEXEC);check(fd>=0 && ftruncate(fd,width*height*4)==0,"buffer storage");
    auto* pixels=static_cast<uint32_t*>(mmap(nullptr,width*height*4,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0));check(pixels!=MAP_FAILED,"mmap");
    std::fill(pixels,pixels+width*height,color);
    if (split) std::fill(pixels+width*height/2,pixels+width*height,0x80008000);
    auto* pool=wl_shm_create_pool(c.shm,fd,width*height*4);
    auto* buffer=wl_shm_pool_create_buffer(pool,0,width,height,width*4,WL_SHM_FORMAT_ARGB8888);
    wl_shm_pool_destroy(pool);close(fd);
    wl_surface_attach(s,buffer,0,0);wl_surface_commit(s);roundtrip(c);
    wl_buffer_destroy(buffer);munmap(pixels,width*height*4);
}
static void stage(Client& c,int send,int receive,char stage) {
    roundtrip(c);check(write(send,&stage,1)==1,"send stage");char reply;check(read(receive,&reply,1)==1 && reply==stage,"stage ack");roundtrip(c);
}
static void client(const char* socket,int send,int receive) {
    Client c;c.display=wl_display_connect(socket);check(c.display,"connect");
    auto* registry=wl_display_get_registry(c.display);wl_registry_add_listener(registry,&globals,&c);roundtrip(c);
    check(c.compositor && c.subcompositor && c.shm && c.shell && c.seat,"tree globals");
    auto* pointer=wl_seat_get_pointer(c.seat);wl_pointer_add_listener(pointer,&pointer_listener,&c);wl_seat_release(c.seat);
    auto* root=wl_compositor_create_surface(c.compositor);
    auto* xdg=xdg_wm_base_get_xdg_surface(c.shell,root);xdg_surface_add_listener(xdg,&xdg_listener,nullptr);
    auto* top=xdg_surface_get_toplevel(xdg);xdg_toplevel_add_listener(top,&top_listener,nullptr);
    wl_surface_commit(root);roundtrip(c);fill(c,root,0xffff0000,4,4);
    auto* child=wl_compositor_create_surface(c.compositor);
    auto* sub=wl_subcompositor_get_subsurface(c.subcompositor,child,root);
    wl_subsurface_set_position(sub,1,1);
    wl_callback_add_listener(wl_surface_frame(child),&frame_listener,&c);
    fill(c,child,0xff0000ff);stage(c,send,receive,'A');check(c.frames==0,"sync callback must wait for parent");
    wl_surface_commit(root);stage(c,send,receive,'B');check(c.frames==1,"parent releases child callback");
    wl_subsurface_set_position(sub,2,0);fill(c,child,0xff00ff00,4,4);stage(c,send,receive,'C');
    wl_subsurface_set_desync(sub);stage(c,send,receive,'D');
    wl_surface_commit(root);stage(c,send,receive,'E');
    check(c.entered==child && c.x==0.5 && c.y==0.5 && c.buttons==1,"child hit and button grab");
    check(c.pointer_frames==3 && c.axis_sources==1 && c.axes==2,"v5 enter/button/axes frames");
    stage(c,send,receive,'F');check(c.entered==child && c.x==-0.5 && c.y==3.5 && c.buttons==2,"grab retains child coordinates outside child");
    wl_subsurface_place_below(sub,root);wl_surface_commit(root);stage(c,send,receive,'G');
    wl_subsurface_place_above(sub,root);wl_subsurface_set_position(sub,-1,-1);wl_surface_commit(root);stage(c,send,receive,'H');
    auto* empty=wl_compositor_create_region(c.compositor);wl_surface_set_input_region(child,empty);wl_region_destroy(empty);wl_surface_commit(child);
    stage(c,send,receive,'I');check(c.entered==root,"empty child input region passes through to root");
    // Region is copied, double buffered, and honors ordered subtraction/addition.
    auto* region=wl_compositor_create_region(c.compositor);wl_region_add(region,0,0,2,2);wl_region_subtract(region,0,0,2,2);wl_region_add(region,1,1,1,1);
    wl_surface_set_input_region(child,region);wl_region_destroy(region);wl_surface_commit(child);
    stage(c,send,receive,'J');check(c.entered==child && c.x==1.5 && c.y==1.5,"region subtraction/addition");
    wl_subsurface_set_sync(sub);
    auto* grand=wl_compositor_create_surface(c.compositor);auto* grand_sub=wl_subcompositor_get_subsurface(c.subcompositor,grand,child);
    wl_subsurface_set_desync(grand_sub);fill(c,grand,0xffffffff,1,1);wl_surface_commit(child);wl_subsurface_set_position(grand_sub,1,0);stage(c,send,receive,'K');
    wl_surface_commit(root);stage(c,send,receive,'L');
    // Desync is overridden by any synchronized ancestor. Parent destruction
    // detaches the hierarchy even while a child owns a pointer grab.
    wl_subsurface_destroy(grand_sub);wl_surface_destroy(grand);
    wl_subsurface_set_desync(sub);wl_surface_set_input_region(child,nullptr);
    wl_surface_set_buffer_scale(child,2);wl_surface_set_buffer_transform(child,1);
    wl_subsurface_set_position(sub,0,0);wl_surface_commit(root);
    fill(c,child,0x80000080,2,4,true);stage(c,send,receive,'M');
    wl_surface_destroy(child);stage(c,send,receive,'N');
    wl_subsurface_destroy(sub);
    auto* orphan=wl_compositor_create_surface(c.compositor);auto* orphan_sub=wl_subcompositor_get_subsurface(c.subcompositor,orphan,root);
    fill(c,orphan,0xffffffff);wl_surface_commit(root);
    xdg_toplevel_destroy(top);xdg_surface_destroy(xdg);wl_surface_destroy(root);stage(c,send,receive,'O');
    wl_subsurface_destroy(orphan_sub);wl_surface_destroy(orphan);
    wl_pointer_release(pointer);roundtrip(c);wl_display_disconnect(c.display);
    for (int invalid=0; invalid<3; ++invalid) {
        Client bad;bad.display=wl_display_connect(socket);check(bad.display,"invalid trial connect");
        auto* registry2=wl_display_get_registry(bad.display);wl_registry_add_listener(registry2,&globals,&bad);roundtrip(bad);
        auto* a=wl_compositor_create_surface(bad.compositor);auto* b=wl_compositor_create_surface(bad.compositor);
        auto* sub2=wl_subcompositor_get_subsurface(bad.subcompositor,b,a);
        if (invalid==0) wl_subcompositor_get_subsurface(bad.subcompositor,a,b);
        if (invalid==1) wl_subcompositor_get_subsurface(bad.subcompositor,b,a);
        if (invalid==2) {
            auto* stranger=wl_compositor_create_surface(bad.compositor);
            wl_subsurface_place_above(sub2,stranger);
        }
        check(wl_display_roundtrip(bad.display)<0 && wl_display_get_error(bad.display)==EPROTO,"invalid relationship rejected");
        const wl_interface* interface=nullptr;
        auto code=wl_display_get_protocol_error(bad.display,&interface,nullptr);
        check(interface && !std::strcmp(interface->name,invalid==2?"wl_subsurface":"wl_subcompositor") &&
              code==uint32_t(invalid==0?WL_SUBCOMPOSITOR_ERROR_BAD_PARENT:0),"exact tree protocol error");
        wl_display_disconnect(bad.display);
    }
    _exit(0);
}
static void pixel(const elsewhere::OwnedFrame& f,int x,int y,int r,int g,int b) {
    check(f && f->mapped && x<f->width && y<f->height,"mapped pixel bounds");
    auto* p=&f->pixels[(y*f->width+x)*4];
    check(std::abs(int(p[0])-r)<=1 && std::abs(int(p[1])-g)<=1 && std::abs(int(p[2])-b)<=1,"composed pixel");
}
int main() {
    alarm(25);char directory[]="/tmp/elsewhere-tree-XXXXXX";check(mkdtemp(directory),"directory");
    elsewhere::CompositorRuntime runtime;check(runtime.start(directory),"runtime start");
    int to_parent[2],to_child[2];check(pipe2(to_parent,O_CLOEXEC)==0 && pipe2(to_child,O_CLOEXEC)==0,"pipes");
    pid_t pid=fork();check(pid>=0,"fork");
    if (!pid) { close(to_parent[0]);close(to_child[1]);client(runtime.socket_path().c_str(),to_parent[1],to_child[0]); }
    close(to_parent[1]);close(to_child[0]);fcntl(to_parent[0],F_SETFL,O_NONBLOCK);
    int64_t handle=0;uint64_t revision=0;elsewhere::OwnedFrame retained;
    for (char expected='A';expected<='O';++expected) {
        char stage;ssize_t count;
        while ((count=read(to_parent[0],&stage,1))<0 && errno==EAGAIN) {check(runtime.pump(),"pump");usleep(1000);}
        check(count==1 && stage==expected,"ordered stage");
        if (!handle) {check(runtime.toplevel_handles().size()==1,"one root");handle=runtime.toplevel_handles()[0];}
        auto f=runtime.snapshot(handle);
        if (stage<'O') check(f && f->mapped,"root mapped");
        switch(stage) {
        case 'A': pixel(f,1,1,255,0,0);break;
        case 'B': pixel(f,1,1,0,0,255);retained=f;revision=f->revision;break;
        case 'C': pixel(f,1,1,0,0,255);check(f->revision==revision,"cached child does not change visible revision");break;
        case 'D': pixel(f,1,1,0,255,0);pixel(f,3,0,255,0,0);check(runtime.window_state(handle)->width==5,"default geometry follows desynchronized child resize");check(f->revision>revision,"desync publishes new revision");break;
        case 'E': pixel(f,2,0,0,255,0);pixel(f,1,1,255,0,0);check(runtime.pointer_motion(handle,2.5,0.5) && runtime.pointer_button(272,true),"child pointer press");check(runtime.pointer_axis(10,20),"v5 wheel axes");break;
        case 'F': check(runtime.pointer_motion(handle,1.5,3.5) && runtime.pointer_button(272,false),"child pointer grabbed motion/release");break;
        case 'G': pixel(f,2,0,255,0,0);break;
        case 'H': check(f->origin_x==-1 && f->origin_y==-1 && f->width==5 && f->height==5,"negative child expands canvas");pixel(f,0,0,0,255,0);check(runtime.window_state(handle)->width==5,"default geometry includes child bounds");break;
        case 'I': check(runtime.pointer_motion(handle,1.5,1.5),"input hole routing");break;
        case 'J': check(runtime.pointer_motion(handle,1.5,1.5),"region routing");break;
        case 'K': pixel(f,0,0,0,255,0);break;
        case 'L': pixel(f,0,0,255,255,255);pixel(f,1,0,0,255,0);break;
        case 'M': check(f->width==4 && f->height==4,"transformed scaled child extent");pixel(f,0,0,127,128,0);pixel(f,1,0,127,0,128);check(runtime.pointer_motion(handle,0.5,0.5) && runtime.pointer_button(272,true),"grab before destroy");break;
        case 'N': pixel(f,0,0,255,0,0);check(!runtime.pointer_grabbed() && runtime.pointer_focus_handle()==0,"destroy clears child grab");break;
        case 'O': check(!f && runtime.toplevel_count()==0,"parent destruction removes root");break;
        }
        if (retained) pixel(retained,1,1,0,0,255);
        check(write(to_child[1],&stage,1)==1,"ack stage");
    }
    int status;while (waitpid(pid,&status,WNOHANG)==0) {check(runtime.pump(),"exit pump");usleep(1000);}
    check(WIFEXITED(status) && WEXITSTATUS(status)==0,"client exit");
    for(int i=0;i<3;++i) check(runtime.pump(),"cleanup pump");
    check(runtime.surface_count()==0 && runtime.client_count()==0,"no leaked protocol state");
    close(to_parent[0]);close(to_child[1]);check(runtime.stop() && rmdir(directory)==0,"runtime cleanup");
    std::puts("SURFACE_TREE_OK stages=15 sync/desync, stacking, regions, transforms, pointer grabs, ownership");
}
