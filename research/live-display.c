/* Persistent lossless capture of the real QuartzCore display. No synthetic UI. */
#include "live-display.h"
#include "frame-damage.h"
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void *surface,*name;
static void (*release)(void *);
static int (*lock_surface)(void *,uint32_t,uint32_t *),(*unlock_surface)(void *,uint32_t,uint32_t *);
static void *(*base)(void *);
static _Bool (*render)(uint32_t,void *,void *,int,int);
static unsigned long (*bound)(unsigned long),(*crc)(unsigned long,const unsigned char *,unsigned);
static int (*compress_bytes)(unsigned char *,unsigned long *,const unsigned char *,unsigned long,int);
static unsigned width,height;
static size_t stride,length,capacity;
static unsigned char *raw,*packed,*patch;
static int baseline;
static unsigned long sequence;
void live_display_close(void) {
    if(surface&&release)release(surface);
    if(name&&release)release(name);
    surface=name=NULL;free(raw);free(packed);free(patch);raw=packed=patch=NULL;baseline=0;
}
static int initialize(void) {
    void *qc=dlopen("/System/Library/Frameworks/QuartzCore.framework/QuartzCore",RTLD_NOW);
    void *io=dlopen("/System/Library/Frameworks/IOSurface.framework/IOSurface",RTLD_NOW);
    void *z=dlopen("/usr/lib/libz.1.dylib",RTLD_NOW);
    if(!qc||!io||!z)return 0;
#define LOAD(variable,handle,symbol) do{*(void **)(&variable)=dlsym(handle,symbol);if(!variable)return 0;}while(0)
    void *(*cls)(const char *)=NULL,*(*sel)(const char *)=NULL,*msg=NULL;
    void *(*create)(void *)=NULL;size_t (*get_stride)(void *)=NULL,(*get_size)(void *)=NULL;
    LOAD(cls,RTLD_DEFAULT,"objc_getClass");LOAD(sel,RTLD_DEFAULT,"sel_registerName");LOAD(msg,RTLD_DEFAULT,"objc_msgSend");
    LOAD(create,io,"IOSurfaceCreate");LOAD(get_stride,io,"IOSurfaceGetBytesPerRow");LOAD(get_size,io,"IOSurfaceGetAllocSize");
    LOAD(lock_surface,io,"IOSurfaceLock");LOAD(unlock_surface,io,"IOSurfaceUnlock");LOAD(base,io,"IOSurfaceGetBaseAddress");
    LOAD(render,qc,"CARenderServerRenderDisplay");LOAD(release,RTLD_DEFAULT,"CFRelease");
    LOAD(bound,z,"compressBound");LOAD(compress_bytes,z,"compress2");LOAD(crc,z,"crc32");
    void *(*get)(void *,void *)=(void *(*)(void *,void *))msg;
    void *display=get(cls("CADisplay"),sel("mainDisplay"));if(!display)return 0;
    void *mode=get(display,sel("currentMode"));if(!mode)return 0;
    unsigned long w=((unsigned long(*)(void *,void *))msg)(mode,sel("width"));
    unsigned long h=((unsigned long(*)(void *,void *))msg)(mode,sel("height"));
    if(!w||!h||w>4096||h>4096||w*h>4*1024*1024)return 0;
    width=(unsigned)w;height=(unsigned)h;length=w*h*4;capacity=bound(length);
    name=get(get(display,sel("name")),sel("copy"));if(!name)return 0;
    void *dict=get(cls("NSMutableDictionary"),sel("dictionary"));
    const char *keys[]={"kIOSurfaceWidth","kIOSurfaceHeight","kIOSurfaceBytesPerElement","kIOSurfaceBytesPerRow","kIOSurfaceAllocSize","kIOSurfacePixelFormat"};
    unsigned values[]={width,height,4,width*4,(unsigned)length,0x42475241};
    for(unsigned i=0;i<6;i++) {
        void **key=dlsym(io,keys[i]);if(!key)goto fail;
        void *n=((void *(*)(void *,void *,unsigned))msg)(cls("NSNumber"),sel("numberWithUnsignedInt:"),values[i]);
        ((void(*)(void *,void *,void *,void *))msg)(dict,sel("setObject:forKey:"),n,*key);
    }
    surface=create(dict);if(!surface)goto fail;
    stride=get_stride(surface);size_t allocation=get_size(surface);
    if(stride<width*4||stride>16384||allocation<stride*height||allocation>16*1024*1024)goto fail;
    raw=malloc(length);packed=malloc(capacity);patch=malloc(length);if(!raw||!packed||!patch)goto fail;
    return 1;
fail:live_display_close();return 0;
}
int live_display_frame(int force_full) {
    if(!surface&&!initialize())return 0;
    if(!render(0,name,surface,0,0)||lock_surface(surface,0,NULL))return 0;
    const unsigned char *pixels=base(surface);
    if(!pixels){unlock_surface(surface,0,NULL);return 0;}
    int full=force_full||!baseline;
    FrameDamage damage=full?(FrameDamage){0,0,width,height}:frame_damage(raw,pixels,width,height,stride);
    size_t patch_length=(size_t)damage.width*damage.height*4;
    for(unsigned y=0;y<damage.height;y++) {
        const unsigned char *row=pixels+(size_t)(damage.y+y)*stride+damage.x*4;
        memcpy(patch+(size_t)y*damage.width*4,row,(size_t)damage.width*4);
        memcpy(raw+(size_t)(damage.y+y)*width*4+damage.x*4,row,(size_t)damage.width*4);
    }
    baseline=0;
    if(unlock_surface(surface,0,NULL))return 0;
    if(!patch_length) {
        printf("\nLIVE_FRAME_SAME %lu\n",sequence);baseline=!ferror(stdout);return baseline;
    }
    /* UART transfer dominates capture latency; reduce bytes without changing
       pixels. V83's UUID-guarded runtime trial exercised this level. */
    unsigned long bytes=capacity;if(compress_bytes(packed,&bytes,patch,patch_length,6))return 0;
    unsigned long previous=sequence,id=++sequence;
    if(full)printf("\nLIVE_FRAME_BEGIN %lu %u %u %zu %lu %08lx\n",id,width,height,patch_length,bytes,crc(0,packed,(unsigned)bytes));
    else printf("\nLIVE_PATCH_BEGIN %lu %lu %u %u %u %u %u %u %zu %lu %08lx\n",id,previous,width,height,damage.x,damage.y,damage.width,damage.height,patch_length,bytes,crc(0,packed,(unsigned)bytes));
    const char alphabet[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    for(unsigned long offset=0;offset<bytes;offset+=96) {
        unsigned n=(unsigned)(bytes-offset);if(n>96)n=96;
        char line[1152];int prefix=snprintf(line,sizeof(line),"LIVE_FRAME_DATA %lu %lu ",id,offset);
        size_t out=(size_t)prefix;
        for(unsigned i=0;i<n;i+=3) {
            unsigned v=(unsigned)packed[offset+i]<<16;
            if(i+1<n)v|=(unsigned)packed[offset+i+1]<<8;
            if(i+2<n)v|=packed[offset+i+2];
            line[out++]=alphabet[(v>>18)&63];line[out++]=alphabet[(v>>12)&63];
            line[out++]=i+1<n?alphabet[(v>>6)&63]:'=';line[out++]=i+2<n?alphabet[v&63]:'=';
        }
        line[out++]='\n';if(fwrite(line,1,out,stdout)!=out)return 0;
    }
    printf("LIVE_FRAME_END %lu\n",id);baseline=!ferror(stdout);return baseline;
}
