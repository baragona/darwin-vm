#include "frame-damage.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    unsigned char old[3*3*4]={0},pixels[3*16]={0};
    FrameDamage d=frame_damage(old,pixels,3,3,16);assert(!d.width&&!d.height);
    pixels[15]=99;d=frame_damage(old,pixels,3,3,16);assert(!d.width); /* padding ignored */
    pixels[16+4]=1;d=frame_damage(old,pixels,3,3,16);
    assert(d.x==1&&d.y==1&&d.width==1&&d.height==1);
    pixels[8]=1;pixels[32]=1;d=frame_damage(old,pixels,3,3,16);
    assert(d.x==0&&d.y==0&&d.width==3&&d.height==3);
    puts("PASS: unchanged pixels, padding, isolated pixel and disjoint damage bounds");
}
