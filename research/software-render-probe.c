/* 24A437-specific opaque software-renderer experiment. No system display changes.
 * ABI traced through CARenderOGLNew_, software callback constructor, and
 * SWContext::set_destination(void*,long,unsigned long,void*,long,int,int,int,int).
 * The empty update is a smoke test, not proof of layer or GUI composition.
 */
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
typedef struct { double x,y,w,h; } Rect;
int main(void) {
    setbuf(stdout,NULL); alarm(20);
    puts("SW_PROBE_BEGIN");
    void *h=dlopen("/System/Library/Frameworks/QuartzCore.framework/QuartzCore",RTLD_NOW);
    if(!h) { puts(dlerror()); return 1; }
    void *callbacks=dlsym(h,"kCARenderSoftwareCallbacks");
    void *(*create)(const void*,void*,void*)=(void*(*)(const void*,void*,void*))dlsym(h,"CARenderOGLNew");
    int (*destination)(void*,void*,long,unsigned long,void*,long,int,int,int,int)=
        (int(*)(void*,void*,long,unsigned long,void*,long,int,int,int,int))dlsym(h,"CARenderSoftwareSetDestination");
    void *(*begin)(void*,size_t,double,const void*,uint32_t,const Rect*)=
        (void*(*)(void*,size_t,double,const void*,uint32_t,const Rect*))dlsym(h,"CARenderUpdateBegin");
    void (*add_rect)(void*,const Rect*)=(void(*)(void*,const Rect*))dlsym(h,"CARenderUpdateAddRect");
    void (*render)(void*,void*)=(void(*)(void*,void*))dlsym(h,"CARenderOGLRender");
    void (*finish)(void*)=(void(*)(void*))dlsym(h,"CARenderOGLFinish");
    void (*update_finish)(void*)=(void(*)(void*))dlsym(h,"CARenderUpdateFinish");
    if(!callbacks||!create||!destination||!begin||!add_rect||!render||!finish||!update_finish) {
        puts("SW_SYMBOLS_MISSING"); return 1;
    }
    puts("SW_CREATE_BEGIN");
    /* Software callback constructor ignores all three forwarded arguments. */
    void *renderer=create(callbacks,NULL,NULL);
    printf("SW_RENDERER_PRESENT=%d\n",renderer!=NULL);
    if(!renderer) return 1;
    enum { W=64,H=64,STRIDE=W*4,SIZE=STRIDE*H,GUARD=4096 };
    unsigned char *allocation=malloc(SIZE+GUARD*2);
    if(!allocation) return 1;
    memset(allocation,0xa5,SIZE+GUARD*2);
    unsigned char *pixels=allocation+GUARD;
    /* Destination setter stores bits/pixel and bounds; verified 24A437 ABI. */
    int result=destination(renderer,pixels,STRIDE,32,NULL,0,0,0,W,H);
    printf("SW_DESTINATION_RESULT=%d\n",result);
    Rect bounds={0,0,W,H};
    void *update=begin(NULL,0,1.0,NULL,0,&bounds);
    printf("SW_UPDATE_PRESENT=%d\n",update!=NULL);
    if(!update) return 1;
    add_rect(update,&bounds);
    puts("SW_RENDER_EMPTY_BEGIN");
    render(renderer,update);
    puts("SW_RENDER_EMPTY_RETURNED");
    finish(renderer);
    puts("SW_FINISH_RETURNED");
    update_finish(update);
    size_t changed=0,guard_changed=0;
    for(size_t i=0;i<SIZE;i++) changed+=pixels[i]!=0xa5;
    for(size_t i=0;i<GUARD;i++) guard_changed+=(allocation[i]!=0xa5)+(pixels[SIZE+i]!=0xa5);
    printf("SW_CHANGED_BYTES=%zu GUARD_CHANGED_BYTES=%zu\n",changed,guard_changed);
    puts("SW_PROBE_END");
    /* Process exit owns private renderer cleanup. */
    return guard_changed?1:0;
}
