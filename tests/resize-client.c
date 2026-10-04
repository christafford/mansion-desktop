#define _GNU_SOURCE
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void check(int ok, const char* message) {
    if (!ok) { fprintf(stderr, "RESIZE CLIENT FAIL: %s\n", message); exit(1); }
}
static struct wl_display* display;
static struct wl_compositor* compositor;
static struct wl_shm* shm;
static struct xdg_wm_base* shell;
static struct wl_surface* surface;
static struct xdg_surface* xdg;
static struct xdg_toplevel* top;
static FILE* events;
static uint32_t latest_serial, first_resize_serial;
static int latest_width, latest_height, count;
static void global(void* data, struct wl_registry* r, uint32_t id, const char* name, uint32_t version) {
    (void)data; (void)version;
    if (!strcmp(name,"wl_compositor")) compositor=wl_registry_bind(r,id,&wl_compositor_interface,4);
    else if (!strcmp(name,"wl_shm")) shm=wl_registry_bind(r,id,&wl_shm_interface,1);
    else if (!strcmp(name,"xdg_wm_base")) shell=wl_registry_bind(r,id,&xdg_wm_base_interface,3);
}
static void removed(void* d, struct wl_registry* r, uint32_t id) { (void)d;(void)r;(void)id; }
static const struct wl_registry_listener registry_listener={global,removed};
static void configured(void* d, struct xdg_surface* s, uint32_t serial) {
    (void)d;(void)s; check(serial != 0,"serial");
    latest_serial=serial;
    if (++count == 2) first_resize_serial=serial;
    fprintf(events,"configure %u %d %d\n",serial,latest_width,latest_height);
}
static const struct xdg_surface_listener surface_listener={configured};
static void top_configured(void* d,struct xdg_toplevel* t,int32_t w,int32_t h,struct wl_array* states) {
    (void)d;(void)t;check(states->size==0,"ordinary size suggestion");latest_width=w;latest_height=h;
}
static void close_top(void* d,struct xdg_toplevel* t) { (void)d;(void)t;check(0,"unexpected close"); }
static const struct xdg_toplevel_listener top_listener={.configure=top_configured,.close=close_top};
static void sync_client(void) { check(wl_display_roundtrip(display)>=0,"roundtrip"); }
static int exists(const char* base,const char* suffix) {
    char path[2048];check(snprintf(path,sizeof path,"%s.%s",base,suffix)<(int)sizeof path,"path");return access(path,F_OK)==0;
}
static void mark(const char* base,const char* suffix) {
    char path[2048];check(snprintf(path,sizeof path,"%s.%s",base,suffix)<(int)sizeof path,"path");
    int fd=open(path,O_CREAT|O_WRONLY|O_CLOEXEC,0600);check(fd>=0,"marker");close(fd);
}
static void pixels(int w,int h,unsigned char value) {
    int size=w*h*4;
    int fd=memfd_create("resize-fixture",MFD_CLOEXEC);check(fd>=0 && ftruncate(fd,size)==0,"shm fd");
    unsigned char* data=mmap(NULL,size,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);check(data!=MAP_FAILED,"mmap");
    memset(data,value,size);
    struct wl_shm_pool* pool=wl_shm_create_pool(shm,fd,size);
    struct wl_buffer* buffer=wl_shm_pool_create_buffer(pool,0,w,h,w*4,WL_SHM_FORMAT_XRGB8888);
    wl_surface_attach(surface,buffer,0,0);wl_surface_commit(surface);
    sync_client();wl_buffer_destroy(buffer);wl_shm_pool_destroy(pool);munmap(data,size);close(fd);
}
static void expect_error(uint32_t code, const char* interface_name) {
    wl_display_flush(display);
    while (wl_display_dispatch(display) >= 0) {}
    const struct wl_interface* interface = NULL;
    uint32_t id = 0;
    check(wl_display_get_protocol_error(display, &interface, &id) == code &&
          interface && !strcmp(interface->name, interface_name) && id != 0, "wrong protocol rejection");
}
int main(int argc,char** argv) {
    check(argc==3,"socket and marker base");alarm(30);
    char path[2048];snprintf(path,sizeof path,"%s.events",argv[2]);events=fopen(path,"w");check(events!=NULL,"log");setvbuf(events,NULL,_IOLBF,0);
    display=wl_display_connect(argv[1]);check(display!=NULL,"connect");
    struct wl_registry* registry=wl_display_get_registry(display);wl_registry_add_listener(registry,&registry_listener,NULL);sync_client();
    check(compositor && shm && shell,"globals");
    surface=wl_compositor_create_surface(compositor);xdg=xdg_wm_base_get_xdg_surface(shell,surface);
    xdg_surface_add_listener(xdg,&surface_listener,NULL);top=xdg_surface_get_toplevel(xdg);xdg_toplevel_add_listener(top,&top_listener,NULL);
    wl_surface_commit(surface);sync_client();check(count==1,"initial configure");
    xdg_surface_ack_configure(xdg,latest_serial);
    xdg_toplevel_set_min_size(top,32,24);xdg_toplevel_set_max_size(top,120,100);
    xdg_surface_set_window_geometry(xdg,4,6,72,48);pixels(80,60,0x11);mark(argv[2],"ready");
    int step=1;
    while (!exists(argv[2],"finish")) {
        char command[32];snprintf(command,sizeof command,"%d",step);
        if (exists(argv[2],command)) {
            switch(step) {
            case 1: // Requests are pending until commit, even after a roundtrip.
                xdg_surface_set_window_geometry(xdg,2,3,44,30);
                xdg_toplevel_set_min_size(top,40,30);xdg_toplevel_set_max_size(top,110,90);break;
            case 2: xdg_surface_ack_configure(xdg,latest_serial);break; // ack only
            case 3: pixels(50,40,0x22);break; // Deliberately choose a different size.
            case 4: xdg_surface_ack_configure(xdg,latest_serial);wl_surface_commit(surface);break; // decline
            case 5: // Coalesce multiple configures, then attach a resized buffer.
                xdg_surface_ack_configure(xdg,latest_serial);
                xdg_surface_set_window_geometry(xdg,5,7,100,70);pixels(110,84,0x33);break;
            case 6: // Geometry-only commit; do not copy/replace pixels.
                xdg_surface_set_window_geometry(xdg,-10,-20,1000,1000);wl_surface_commit(surface);break;
            case 7: pixels(120,90,0x44);break; // Explicit geometry stays as previously clamped.
            case 8: xdg_toplevel_destroy(top);xdg_surface_destroy(xdg);wl_surface_destroy(surface);top=NULL;break;
            default: check(0,"unknown step");
            }
            sync_client();snprintf(command,sizeof command,"%d-ok",step++);mark(argv[2],command);
        }
        if (exists(argv[2],"stale")) { xdg_surface_ack_configure(xdg,first_resize_serial);expect_error(XDG_SURFACE_ERROR_INVALID_SERIAL,"xdg_surface");return 0; }
        if (exists(argv[2],"negative")) { xdg_toplevel_set_min_size(top,-1,0);expect_error(XDG_TOPLEVEL_ERROR_INVALID_SIZE,"xdg_toplevel");return 0; }
        if (exists(argv[2],"conflict")) { xdg_toplevel_set_min_size(top,121,101);wl_surface_commit(surface);expect_error(XDG_TOPLEVEL_ERROR_INVALID_SIZE,"xdg_toplevel");return 0; }
        if (exists(argv[2],"outside")) { xdg_surface_set_window_geometry(xdg,2147483647,2147483647,2147483647,2147483647);wl_surface_commit(surface);expect_error(XDG_SURFACE_ERROR_INVALID_SIZE,"xdg_surface");return 0; }
        sync_client();usleep(1000);
    }
    if(top){xdg_toplevel_destroy(top);xdg_surface_destroy(xdg);wl_surface_destroy(surface);}
    xdg_wm_base_destroy(shell);wl_shm_destroy(shm);wl_compositor_destroy(compositor);sync_client();
    wl_registry_destroy(registry);wl_display_disconnect(display);fclose(events);mark(argv[2],"ok");return 0;
}
