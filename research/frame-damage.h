#ifndef DARWINVM_FRAME_DAMAGE_H
#define DARWINVM_FRAME_DAMAGE_H
#include <stddef.h>
#include <string.h>
typedef struct { unsigned x,y,width,height; } FrameDamage;
/* Both buffers contain BGRA pixels; the captured surface may have row padding. */
static inline FrameDamage frame_damage(const unsigned char *old,const unsigned char *pixels,
                                      unsigned width,unsigned height,size_t stride) {
    unsigned left=width,top=height,right=0,bottom=0;
    for(unsigned y=0;y<height;y++) {
        const unsigned char *a=old+(size_t)y*width*4,*b=pixels+(size_t)y*stride;
        if(!memcmp(a,b,(size_t)width*4))continue;
        if(top==height)top=y;bottom=y+1;
        unsigned x=0;while(x<width&&!memcmp(a+x*4,b+x*4,4))x++;
        if(x<left)left=x;
        x=width;while(x>left&&!memcmp(a+(x-1)*4,b+(x-1)*4,4))x--;
        if(x>right)right=x;
    }
    if(top==height)return (FrameDamage){0,0,0,0};
    return (FrameDamage){left,top,right-left,bottom-top};
}
#endif
