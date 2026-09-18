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
int main(int argc, char **argv) {
    int layer_mode=argc==2 && !strcmp(argv[1],"--layer");
    if(argc>1 && !layer_mode) { puts("usage: software-render-probe [--layer]"); return 2; }
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
    void *context=NULL;
    double time=1.0;
    if(layer_mode) {
        void *(*cls)(const char*)=(void*(*)(const char*))dlsym(RTLD_DEFAULT,"objc_getClass");
        void *(*sel)(const char*)=(void*(*)(const char*))dlsym(RTLD_DEFAULT,"sel_registerName");
        void *msg=dlsym(RTLD_DEFAULT,"objc_msgSend");
        void *(*space_create)(void)=(void*(*)(void))dlsym(RTLD_DEFAULT,"CGColorSpaceCreateDeviceRGB");
        void *(*color_create)(void*,const double*)=(void*(*)(void*,const double*))dlsym(RTLD_DEFAULT,"CGColorCreate");
        double (*now)(void)=(double(*)(void))dlsym(h,"CACurrentMediaTime");
        if(!cls||!sel||!msg||!space_create||!color_create||!now) { puts("SW_LAYER_SYMBOLS_MISSING"); return 1; }
        void *(*get)(void*,void*)=(void*(*)(void*,void*))msg;
        void (*send)(void*,void*)=(void(*)(void*,void*))msg;
        puts("SW_LOCAL_CONTEXT_BEGIN");
        void *local=get(cls("CAContext"),sel("localContext"));
        printf("SW_LOCAL_CONTEXT_PRESENT=%d\n",local!=NULL);
        if(!local) return 1;
        send(cls("CATransaction"),sel("begin"));
        ((void(*)(void*,void*,_Bool))msg)(cls("CATransaction"),sel("setDisableActions:"),1);
        void *layer=get(cls("CALayer"),sel("layer"));
        ((void(*)(void*,void*,Rect))msg)(layer,sel("setFrame:"),bounds);
        double rgba[]={1,0,0,1};
        void *color=color_create(space_create(),rgba);
        if(!layer||!color) { puts("SW_LAYER_OR_COLOR_MISSING"); return 1; }
        ((void(*)(void*,void*,void*))msg)(layer,sel("setBackgroundColor:"),color);
        ((void(*)(void*,void*,void*))msg)(local,sel("setLayer:"),layer);
        puts("SW_LAYER_COMMIT_BEGIN");
        send(cls("CATransaction"),sel("commit"));
        send(cls("CATransaction"),sel("flush"));
        puts("SW_LAYER_COMMIT_RETURNED");
        context=get(local,sel("renderContext"));
        printf("SW_RENDER_CONTEXT_PRESENT=%d\n",context!=NULL);
        if(!context) return 1;
        time=now();
    }
    void *update=begin(NULL,0,time,NULL,0,&bounds);
    printf("SW_UPDATE_PRESENT=%d\n",update!=NULL);
    if(!update) return 1;
    add_rect(update,&bounds);
    if(layer_mode) {
        void (*add_context)(void*,void*)=(void(*)(void*,void*))dlsym(h,"CARenderUpdateAddContext");
        if(!add_context) return 1;
        puts("SW_ADD_CONTEXT_BEGIN");
        add_context(update,context);
        puts("SW_ADD_CONTEXT_RETURNED");
    }
    puts(layer_mode?"SW_RENDER_LAYER_BEGIN":"SW_RENDER_EMPTY_BEGIN");
    render(renderer,update);
    puts(layer_mode?"SW_RENDER_LAYER_RETURNED":"SW_RENDER_EMPTY_RETURNED");
    finish(renderer);
    puts("SW_FINISH_RETURNED");
    update_finish(update);
    size_t changed=0,guard_changed=0;
    for(size_t i=0;i<SIZE;i++) changed+=pixels[i]!=0xa5;
    for(size_t i=0;i<GUARD;i++) guard_changed+=(allocation[i]!=0xa5)+(pixels[SIZE+i]!=0xa5);
    printf("SW_CHANGED_BYTES=%zu GUARD_CHANGED_BYTES=%zu\n",changed,guard_changed);
    printf("SW_FIRST_PIXEL=%02x%02x%02x%02x CENTER=%02x%02x%02x%02x\n",
        pixels[0],pixels[1],pixels[2],pixels[3],pixels[SIZE/2],pixels[SIZE/2+1],pixels[SIZE/2+2],pixels[SIZE/2+3]);
    if(layer_mode && changed && !guard_changed) {
        FILE *output=fopen("/private/var/tmp/software-layer.raw","wb");
        if(!output) { perror("output"); return 1; }
        size_t written=fwrite(pixels,1,SIZE,output);
        int closed=fclose(output);
        printf("SW_OUTPUT_BYTES=%zu CLOSE_RESULT=%d\n",written,closed);
    }
    puts("SW_PROBE_END");
    /* Process exit owns private renderer cleanup. */
    return guard_changed?1:0;
}
