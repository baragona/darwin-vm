/* Guest-only persistent input endpoint. Dispatch acceptance is not UI delivery. */
#include <dlfcn.h>
#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <signal.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>
#include "virtual-touch-service.h"
#include "live-display.h"

static void *(*hand)(void *,uint64_t,unsigned,unsigned,unsigned,unsigned,unsigned,double,double,double,double,double,unsigned,unsigned,unsigned);
static void *(*finger)(void *,uint64_t,unsigned,unsigned,unsigned,double,double,double,double,double,unsigned,unsigned,unsigned);
static void *(*keyboard)(void *,uint64_t,unsigned,unsigned,unsigned char,unsigned);
static void (*integer)(void *,unsigned,long),(*sender)(void *,uint64_t),(*timestamp)(void *,uint64_t),(*release)(void *),(*append)(void *,void *,unsigned);
static void (*dispatch)(void *,void *);
static uint64_t (*now)(void);
static int (*runloop)(void *,double,unsigned char);
static void *client,*mode;
static uint64_t identity;
static int touching;
static double last_x,last_y;
static unsigned char keys[256];
static volatile sig_atomic_t stopping;
static void stop_signal(int number){(void)number;stopping=1;}
static double seconds(void){struct timespec t;if(clock_gettime(CLOCK_MONOTONIC,&t))return -1;return t.tv_sec+t.tv_nsec/1e9;}

static int touch(double x,double y,int down,int moving) {
    unsigned mask=moving?4:3;
    uint64_t stamp=now();
    void *h=hand(NULL,stamp,3,0,0,mask,0,x,y,0,0,0,down,down,0);
    void *f=finger(NULL,stamp,1,1,mask,x,y,0,down?1.0:0.0,0,down,down,0);
    if(!h||!f){if(h)release(h);if(f)release(f);return 0;}
    integer(h,0xb0019,1);integer(f,0xb0019,1);sender(h,identity);sender(f,identity);append(h,f,0);
    int ok=virtual_touch_dispatch(h);release(f);release(h);
    if(ok){touching=down;last_x=x;last_y=y;}
    return ok;
}
static int key(unsigned page,unsigned usage,int down) {
    void *e=keyboard(NULL,now(),page,usage,(unsigned char)down,0);
    if(!e)return 0;
    sender(e,identity);dispatch(client,e);release(e);
    if(page==7)keys[usage]=(unsigned char)down;
    return 1;
}
static int clear_input(void) {
    int ok=1;
    if(touching&&!touch(last_x,last_y,0,0))ok=0;
    for(unsigned i=0;i<256;i++)if(keys[i]&&!key(7,i,0))ok=0;
    return ok;
}
static int command(char *line) {
    char op=0,extra=0;double x=0,y=0;char u[32],d[32];
    if(sscanf(line," %c %lf %lf %c",&op,&x,&y,&extra)==3 && (op=='D'||op=='M'||op=='U')) {
        if(!isfinite(x)||!isfinite(y)||x<0||x>1||y<0||y>1)return 0;
        if((op=='D'&&touching)||(op!='D'&&!touching))return 0;
        return touch(x,y,op!='U',op=='M');
    }
    if(sscanf(line," K %31s %31s %c",u,d,&extra)==2) {
        if(strspn(u,"0123456789")!=strlen(u)||strspn(d,"01")!=1||strlen(d)!=1)return 0;
        errno=0;unsigned long usage=strtoul(u,NULL,10);
        if(errno||usage<4||usage>231)return 0;
        return key(7,(unsigned)usage,d[0]-'0');
    }
    if(!strcmp(line,"H")) {
        void *down=keyboard(NULL,now(),12,64,1,0),*up=keyboard(NULL,now(),12,64,0,0);
        if(!down||!up){if(down)release(down);if(up)release(up);return 0;}
        sender(down,identity);sender(up,identity);dispatch(client,down);
        (void)runloop(mode,0.04,0);timestamp(up,now());dispatch(client,up);
        release(down);release(up);return 1;
    }
    if(!strcmp(line,"R"))return clear_input();
    if(!strcmp(line,"P"))return 1;
    if(!strcmp(line,"F"))return live_display_frame();
    if(!strcmp(line,"Q")){stopping=1;return 1;}
    return 0;
}
int main(int argc,char **argv) {
    (void)argv;if(argc!=1)return 2;setbuf(stdout,NULL);
    void *io=dlopen("/System/Library/Frameworks/IOKit.framework/IOKit",RTLD_NOW);
    void *cf=dlopen("/System/Library/Frameworks/CoreFoundation.framework/CoreFoundation",RTLD_NOW);
    if(!io||!cf)return 1;
#define LOAD(variable,handle,name) do{*(void **)(&variable)=dlsym(handle,name);if(!variable){printf("LIVE_INPUT_MISSING %s\n",name);return 1;}}while(0)
    LOAD(hand,io,"IOHIDEventCreateDigitizerEvent");LOAD(finger,io,"IOHIDEventCreateDigitizerFingerEvent");
    LOAD(keyboard,io,"IOHIDEventCreateKeyboardEvent");LOAD(integer,io,"IOHIDEventSetIntegerValue");
    LOAD(sender,io,"IOHIDEventSetSenderID");LOAD(timestamp,io,"IOHIDEventSetTimeStamp");LOAD(append,io,"IOHIDEventAppendEvent");
    LOAD(dispatch,io,"IOHIDEventSystemClientDispatchEvent");LOAD(release,cf,"CFRelease");
    LOAD(runloop,cf,"CFRunLoopRunInMode");LOAD(now,RTLD_DEFAULT,"mach_absolute_time");
    void *(*create)(void *,unsigned,void *)=NULL;LOAD(create,io,"IOHIDEventSystemClientCreateWithType");
    void **mode_address=dlsym(cf,"kCFRunLoopDefaultMode");if(!mode_address)return 1;mode=*mode_address;
    client=create(NULL,1,NULL);if(!client)return 1;
    identity=virtual_touch_start();if(!identity){release(client);return 1;}
    struct sigaction action={0};action.sa_handler=stop_signal;
    sigemptyset(&action.sa_mask);sigaction(SIGTERM,&action,NULL);sigaction(SIGINT,&action,NULL);
    double activity=seconds();if(activity<0){virtual_touch_stop();release(client);return 1;}
    puts("LIVE_INPUT_READY 1");
    char line[128];size_t length=0;int overflow=0,result=0;
    while(!stopping) {
        fd_set readable;FD_ZERO(&readable);FD_SET(STDIN_FILENO,&readable);struct timeval wait={0,10000};
        int ready=select(STDIN_FILENO+1,&readable,NULL,NULL,&wait);
        if(ready<0){if(errno==EINTR)continue;result=1;break;}
        if(ready) {
            char bytes[128];ssize_t n=read(STDIN_FILENO,bytes,sizeof(bytes));
            if(n==0)break;
            if(n<0){if(errno==EINTR)continue;result=1;break;}
            for(ssize_t i=0;i<n;i++) {
                if(bytes[i]=='\n') {
                    line[length]=0;
                    int ok=!overflow&&command(line);
                    if(ok&&strchr("DMUKHR",line[0]))activity=seconds();
                    printf("LIVE_INPUT_RESULT %d\n",ok);length=0;overflow=0;
                } else if(bytes[i]!='\r') {
                    if(bytes[i]==0||length==sizeof(line)-1)overflow=1;
                    if(!overflow)line[length++]=bytes[i];
                }
            }
        }
        double tick=seconds();
        if(tick<0){result=1;break;}
        if(tick-activity>5){if(!clear_input())result=1;activity=tick;}
        (void)runloop(mode,0.001,0);
    }
    if(!clear_input())result=1;virtual_touch_stop();live_display_close();release(client);puts("LIVE_INPUT_CLOSED");return result;
}
