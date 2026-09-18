/* 24A437-specific opaque software-renderer experiment. No system display changes.
 * ABI traced through CARenderOGLNew_, software callback constructor, and
 * SWContext::set_destination(void*,long,unsigned long,void*,long,int,int,int,int).
 * Empty, solid-layer, bitmap, and two-frame composition modes remain isolated from UI.
 */
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
typedef struct { double x,y,w,h; } Rect;
int main(int argc, char **argv) {
    int image_mode=argc==3 && !strcmp(argv[1],"--image");
    int bitmap_mode=argc==2 && !strcmp(argv[1],"--bitmap");
    int scene_mode=argc==2 && !strcmp(argv[1],"--scene");
    int layer_mode=image_mode || bitmap_mode || scene_mode || (argc==2 && !strcmp(argv[1],"--layer"));
    if(argc>1 && !layer_mode) { puts("usage: software-render-probe [--layer | --scene | --bitmap | --image PATH]"); return 2; }
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
    void *child=NULL;
    void *(*cls)(const char*)=(void*(*)(const char*))dlsym(RTLD_DEFAULT,"objc_getClass");
        void *(*sel)(const char*)=(void*(*)(const char*))dlsym(RTLD_DEFAULT,"sel_registerName");
        void *msg=dlsym(RTLD_DEFAULT,"objc_msgSend");
        void *(*space_create)(void)=(void*(*)(void))dlsym(RTLD_DEFAULT,"CGColorSpaceCreateDeviceRGB");
        void *(*color_create)(void*,const double*)=(void*(*)(void*,const double*))dlsym(RTLD_DEFAULT,"CGColorCreate");
        double (*now)(void)=(double(*)(void))dlsym(h,"CACurrentMediaTime");
    unsigned char bitmap[64*64*4];
    if(layer_mode) {
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
        if(bitmap_mode) {
            /* Explicit RGBA bytes, opaque primaries, no asset decoding or service lookup.
             * Keep provider bytes alive through commit and render. */
            void *(*provider_create)(void*,const void*,size_t,void(*)(void*,const void*,size_t))=
                (void*(*)(void*,const void*,size_t,void(*)(void*,const void*,size_t)))
                dlsym(RTLD_DEFAULT,"CGDataProviderCreateWithData");
            void *(*image_create)(size_t,size_t,size_t,size_t,size_t,void*,uint32_t,void*,const double*,_Bool,int)=
                (void*(*)(size_t,size_t,size_t,size_t,size_t,void*,uint32_t,void*,const double*,_Bool,int))
                dlsym(RTLD_DEFAULT,"CGImageCreate");
            if(!provider_create||!image_create) { puts("SW_BITMAP_SYMBOLS_MISSING"); return 1; }
            const unsigned char colors[4][4]={{255,0,0,255},{0,255,0,255},{0,0,255,255},{255,255,0,255}};
            for(size_t y=0;y<64;y++) for(size_t x=0;x<64;x++)
                memcpy(bitmap+(y*64+x)*4,colors[(y>=32)*2+(x>=32)],4);
            void *provider=provider_create(NULL,bitmap,sizeof(bitmap),NULL);
            /* kCGBitmapByteOrder32Big | kCGImageAlphaPremultipliedLast */
            void *image=provider?image_create(64,64,8,32,256,space_create(),0x4001,provider,NULL,0,0):NULL;
            printf("SW_BITMAP_IMAGE_PRESENT=%d\n",image!=NULL);
            if(!image) return 1;
            ((void(*)(void*,void*,void*))msg)(layer,sel("setContents:"),image);
        }
        if(image_mode) {
            void *imageio=dlopen("/System/Library/Frameworks/ImageIO.framework/ImageIO",RTLD_NOW);
            void *(*url_create)(void*,const unsigned char*,long,_Bool)=
                (void*(*)(void*,const unsigned char*,long,_Bool))dlsym(RTLD_DEFAULT,"CFURLCreateFromFileSystemRepresentation");
            void *(*source_create)(void*,void*)=imageio?(void*(*)(void*,void*))dlsym(imageio,"CGImageSourceCreateWithURL"):NULL;
            void *(*decode)(void*,size_t,void*)=imageio?(void*(*)(void*,size_t,void*))dlsym(imageio,"CGImageSourceCreateImageAtIndex"):NULL;
            size_t (*width)(void*)=(size_t(*)(void*))dlsym(RTLD_DEFAULT,"CGImageGetWidth");
            size_t (*height)(void*)=(size_t(*)(void*))dlsym(RTLD_DEFAULT,"CGImageGetHeight");
            if(!url_create||!source_create||!decode||!width||!height) { puts("SW_IMAGE_SYMBOLS_MISSING"); return 1; }
            void *url=url_create(NULL,(const unsigned char*)argv[2],(long)strlen(argv[2]),0);
            void *source=url?source_create(url,NULL):NULL;
            printf("SW_IMAGE_SOURCE_PRESENT=%d\n",source!=NULL);
            void *image=source?decode(source,0,NULL):NULL;
            printf("SW_IMAGE_DECODED=%d\n",image!=NULL);
            if(!image) return 1;
            printf("SW_IMAGE_WIDTH=%zu HEIGHT=%zu\n",width(image),height(image));
            ((void(*)(void*,void*,void*))msg)(layer,sel("setContents:"),image);
        }
        if(scene_mode) {
            child=get(cls("CALayer"),sel("layer"));
            Rect child_frame={8,12,24,16};
            double blue[]={0,0,1,0.5};
            void *child_color=color_create(space_create(),blue);
            if(!child||!child_color) return 1;
            ((void(*)(void*,void*,Rect))msg)(child,sel("setFrame:"),child_frame);
            ((void(*)(void*,void*,void*))msg)(child,sel("setBackgroundColor:"),child_color);
            ((void(*)(void*,void*,void*))msg)(layer,sel("addSublayer:"),child);
        }
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
    size_t total_guard_changed=0;
    int bitmap_ok=1;
    for(int frame=0;frame<(scene_mode?2:1);frame++) {
    if(frame==1) {
        void (*send)(void*,void*)=(void(*)(void*,void*))msg;
        send(cls("CATransaction"),sel("begin"));
        ((void(*)(void*,void*,_Bool))msg)(cls("CATransaction"),sel("setDisableActions:"),1);
        Rect child_frame={32,28,24,16};
        double green[]={0,1,0,1};
        void *color=color_create(space_create(),green);
        if(!color) return 1;
        ((void(*)(void*,void*,Rect))msg)(child,sel("setFrame:"),child_frame);
        ((void(*)(void*,void*,void*))msg)(child,sel("setBackgroundColor:"),color);
        send(cls("CATransaction"),sel("commit"));
        send(cls("CATransaction"),sel("flush"));
        time=now();
    }
    printf("SW_FRAME=%d\n",frame);
    /* Keep previous pixels on frame 1: stale content must be repainted. */
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
    if(bitmap_mode) {
        size_t counts[4]={0},other=0;
        const unsigned char bgra[4][4]={{0,0,255,255},{0,255,0,255},{255,0,0,255},{0,255,255,255}};
        for(size_t i=0;i<SIZE;i+=4) {
            unsigned c=0;
            for(;c<4;c++) if(!memcmp(pixels+i,bgra[c],4)) { counts[c]++; break; }
            other+=c==4;
        }
        bitmap_ok=!other;
        for(unsigned c=0;c<4;c++) bitmap_ok&=counts[c]==1024;
        printf("SW_BITMAP_RED=%zu GREEN=%zu BLUE=%zu YELLOW=%zu OTHER=%zu PASS=%d\n",
            counts[0],counts[1],counts[2],counts[3],other,bitmap_ok);
    }
    if(layer_mode && changed && !guard_changed) {
        const char *path=image_mode?"/private/var/tmp/software-image.raw":bitmap_mode?"/private/var/tmp/software-bitmap.raw":scene_mode?(frame?"/private/var/tmp/software-scene-1.raw":"/private/var/tmp/software-scene-0.raw"):"/private/var/tmp/software-layer.raw";
        FILE *output=fopen(path,"wb");
        if(!output) { perror("output"); return 1; }
        size_t written=fwrite(pixels,1,SIZE,output);
        int closed=fclose(output);
        printf("SW_OUTPUT_BYTES=%zu CLOSE_RESULT=%d\n",written,closed);
        if(written!=SIZE || closed) return 1;
    }
    total_guard_changed+=guard_changed;
    }
    puts("SW_PROBE_END");
    /* Process exit owns private renderer cleanup. */
    return total_guard_changed||!bitmap_ok?1:0;
}
