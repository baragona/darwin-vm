/* 24A437 render-server experiment: submit and capture only our own CALayer.
 * CARenderServerRenderLayer ABI traced through 0x1846dbdb0/0x1846db1e8:
 * server port, context ID, CALayer*, IOSurfaceRef, x offset, y offset -> bool.
 */
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
typedef struct { double x,y,w,h; } Rect;
int main(int argc, char **argv) {
    int context_only=argc==2 && !strcmp(argv[1],"--context-only");
    if(argc>1&&!context_only) return 2;
    setbuf(stdout,NULL); alarm(25); puts("REMOTE_PROBE_BEGIN");
    void *qc=dlopen("/System/Library/Frameworks/QuartzCore.framework/QuartzCore",RTLD_NOW);
    void *io=dlopen("/System/Library/Frameworks/IOSurface.framework/IOSurface",RTLD_NOW);
    if(!qc||!io) { printf("DLOPEN_ERROR=%s\n",dlerror()); return 1; }
    void *(*cls)(const char*)=(void*(*)(const char*))dlsym(RTLD_DEFAULT,"objc_getClass");
    void *(*sel)(const char*)=(void*(*)(const char*))dlsym(RTLD_DEFAULT,"sel_registerName");
    void *msg=dlsym(RTLD_DEFAULT,"objc_msgSend");
    void *(*space)(void)=(void*(*)(void))dlsym(RTLD_DEFAULT,"CGColorSpaceCreateDeviceRGB");
    void *(*color)(void*,const double*)=(void*(*)(void*,const double*))dlsym(RTLD_DEFAULT,"CGColorCreate");
    void *(*create)(void*)=(void*(*)(void*))dlsym(io,"IOSurfaceCreate");
    int (*lock)(void*,uint32_t,uint32_t*)=(int(*)(void*,uint32_t,uint32_t*))dlsym(io,"IOSurfaceLock");
    int (*unlock)(void*,uint32_t,uint32_t*)=(int(*)(void*,uint32_t,uint32_t*))dlsym(io,"IOSurfaceUnlock");
    void *(*base)(void*)=(void*(*)(void*))dlsym(io,"IOSurfaceGetBaseAddress");
    size_t (*size)(void*)=(size_t(*)(void*))dlsym(io,"IOSurfaceGetAllocSize");
    size_t (*stride)(void*)=(size_t(*)(void*))dlsym(io,"IOSurfaceGetBytesPerRow");
    _Bool (*render)(uint32_t,uint32_t,void*,void*,int,int)=
        (_Bool(*)(uint32_t,uint32_t,void*,void*,int,int))dlsym(qc,"CARenderServerRenderLayer");
    if(!cls||!sel||!msg||!space||!color||!create||!lock||!unlock||!base||!size||!stride||!render) {
        puts("REMOTE_SYMBOLS_MISSING");return 1;
    }
    void *(*get)(void*,void*)=(void*(*)(void*,void*))msg;
    void (*send)(void*,void*)=(void(*)(void*,void*))msg;
    void *surface=NULL; size_t length=0,row=0; int result=0;
    if(!context_only) {
    void *dict=get(cls("NSMutableDictionary"),sel("dictionary"));
    const char *keys[]={"kIOSurfaceWidth","kIOSurfaceHeight","kIOSurfaceBytesPerElement","kIOSurfaceBytesPerRow","kIOSurfaceAllocSize","kIOSurfacePixelFormat"};
    unsigned values[]={64,64,4,256,16384,0x42475241};
    for(unsigned i=0;i<6;i++) {
        void **key=dlsym(io,keys[i]);if(!key) return 1;
        void *number=((void*(*)(void*,void*,unsigned))msg)(cls("NSNumber"),sel("numberWithUnsignedInt:"),values[i]);
        ((void(*)(void*,void*,void*,void*))msg)(dict,sel("setObject:forKey:"),number,*key);
    }
    puts("REMOTE_SURFACE_CREATE_BEGIN");surface=create(dict);
    printf("REMOTE_SURFACE_PRESENT=%d\n",surface!=NULL);if(!surface) return 1;
    length=size(surface);row=stride(surface);
    printf("REMOTE_SURFACE_SIZE=%zu STRIDE=%zu\n",length,row);
    if(row<256||length<row*64||length>1048576) return 1;
    result=lock(surface,0,NULL);printf("REMOTE_LOCK_RESULT=%d\n",result);if(result) return 1;
    void *pixels=base(surface);if(!pixels) return 1;memset(pixels,0xa5,length);
    result=unlock(surface,0,NULL);if(result) return 1;
    }
    puts("REMOTE_CONTEXT_BEGIN");
    void *context=get(cls("CAContext"),sel("remoteContext"));
    printf("REMOTE_CONTEXT_PRESENT=%d\n",context!=NULL);if(!context) return 1;
    uint32_t context_id=((uint32_t(*)(void*,void*))msg)(context,sel("contextId"));
    printf("REMOTE_CONTEXT_ID=%u\n",context_id);
    send(cls("CATransaction"),sel("begin"));
    ((void(*)(void*,void*,_Bool))msg)(cls("CATransaction"),sel("setDisableActions:"),1);
    void *layer=get(cls("CALayer"),sel("layer"));Rect rect={0,0,64,64};
    ((void(*)(void*,void*,Rect))msg)(layer,sel("setFrame:"),rect);
    double rgba[]={1,0,0,1};void *red=color(space(),rgba);if(!layer||!red) return 1;
    ((void(*)(void*,void*,void*))msg)(layer,sel("setBackgroundColor:"),red);
    ((void(*)(void*,void*,void*))msg)(context,sel("setLayer:"),layer);
    puts("REMOTE_COMMIT_BEGIN");send(cls("CATransaction"),sel("commit"));send(cls("CATransaction"),sel("flush"));
    puts("REMOTE_COMMIT_RETURNED");sleep(1);
    if(context_only) { puts("REMOTE_CONTEXT_ONLY_END"); return 0; }
    puts("REMOTE_SERVER_RENDER_BEGIN");
    printf("REMOTE_SERVER_RENDER_RESULT=%d\n",render(0,context_id,layer,surface,0,0));
    result=lock(surface,0,NULL);printf("REMOTE_READ_LOCK_RESULT=%d\n",result);if(result) return 1;
    unsigned char *bytes=base(surface);size_t changed=0,red_pixels=0;
    for(size_t y=0;y<64;y++) for(size_t x=0;x<64;x++) {
        unsigned char *p=bytes+y*row+x*4;
        for(unsigned b=0;b<4;b++) changed+=p[b]!=0xa5;
        red_pixels+=p[0]==0&&p[1]==0&&p[2]==255&&p[3]==255;
    }
    printf("REMOTE_CHANGED_BYTES=%zu RED_PIXELS=%zu\n",changed,red_pixels);
    printf("REMOTE_READ_UNLOCK_RESULT=%d\n",unlock(surface,0,NULL));
    puts("REMOTE_PROBE_END");return 0;
}
