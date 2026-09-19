/* Minimal real UIKit scene, built without redistributing Apple SDK headers.
 * arm64 iOS ABI: CGFloat=double, NSInteger=long, BOOL=bool.
 * Runtime Objective-C calls keep this buildable with Command Line Tools.
 */
#include <stdbool.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

typedef void *Obj;
typedef struct { double x, y, w, h; } Rect;
static Obj (*cls)(const char *), (*sel)(const char *);
static void *msg;
static Obj window, label, button;
static unsigned taps;
static Obj get(Obj o, const char *s) { return ((Obj(*)(Obj,Obj))msg)(o,sel(s)); }
static Obj arg(Obj o, const char *s, Obj v) { return ((Obj(*)(Obj,Obj,Obj))msg)(o,sel(s),v); }
static void set(Obj o, const char *s, Obj v) { ((void(*)(Obj,Obj,Obj))msg)(o,sel(s),v); }
static Obj str(const char *s) { return ((Obj(*)(Obj,Obj,const char*))msg)(cls("NSString"),sel("stringWithUTF8String:"),s); }
static void frame(Obj o, Rect r) { ((void(*)(Obj,Obj,Rect))msg)(o,sel("setFrame:"),r); }
static void log_event(const char *s) {
    printf("TOUCH_PROBE %s\n",s);
    FILE *f=fopen("/var/mobile/Library/Logs/TouchProbe.log","a");
    if(f) { fprintf(f,"%s\n",s); fclose(f); }
}
static void tapped(Obj self, Obj cmd, Obj sender) {
    (void)self; (void)cmd; (void)sender;
    char text[64]; snprintf(text,sizeof text,"Taps: %u",++taps);
    set(label,"setText:",str(text)); log_event(text);
}
static void layout(Obj self, Obj cmd) {
    (void)cmd;
    Obj view=get(self,"view");
    Rect r=((Rect(*)(Obj,Obj))msg)(view,sel("bounds"));
    frame(label,(Rect){20,r.h*0.30,r.w-40,60});
    frame(button,(Rect){40,r.h*0.50,r.w-80,80});
}
static bool launched(Obj self,Obj cmd,Obj app,Obj options) {
    (void)self;(void)cmd;(void)app;(void)options;
    log_event("DID_FINISH_LAUNCHING"); return true;
}
static void connect_scene(Obj self,Obj cmd,Obj scene,Obj session,Obj options) {
    (void)cmd;(void)session;(void)options;
    log_event("SCENE_CONNECT_BEGIN");
    window=arg(get(cls("UIWindow"),"alloc"),"initWithWindowScene:",scene);
    Obj controller=get(get(cls("TouchViewController"),"alloc"),"init");
    Obj view=get(controller,"view");
    set(view,"setBackgroundColor:",get(cls("UIColor"),"whiteColor"));
    label=get(get(cls("UILabel"),"alloc"),"init");
    set(label,"setText:",str("Taps: 0"));
    set(label,"setTextColor:",get(cls("UIColor"),"blackColor"));
    ((void(*)(Obj,Obj,long))msg)(label,sel("setTextAlignment:"),1);
    Obj font=((Obj(*)(Obj,Obj,double))msg)(cls("UIFont"),sel("systemFontOfSize:"),32.0);
    set(label,"setFont:",font);
    button=((Obj(*)(Obj,Obj,long))msg)(cls("UIButton"),sel("buttonWithType:"),1);
    ((void(*)(Obj,Obj,Obj,unsigned long))msg)(button,sel("setTitle:forState:"),str("Tap me"),0);
    set(button,"setBackgroundColor:",get(cls("UIColor"),"lightGrayColor"));
    set(get(button,"titleLabel"),"setFont:",font);
    ((void(*)(Obj,Obj,Obj,Obj,unsigned long))msg)(button,sel("addTarget:action:forControlEvents:"),self,sel("tapped:"),1UL<<6);
    set(view,"addSubview:",label); set(view,"addSubview:",button);
    set(window,"setRootViewController:",controller);
    ((void(*)(Obj,Obj))msg)(window,sel("makeKeyAndVisible"));
    log_event("SCENE_VISIBLE_REQUESTED");
}
static void scene_active(Obj self,Obj cmd,Obj scene) {
    (void)self;(void)cmd;(void)scene;log_event("SCENE_ACTIVE");
}
int main(int argc,char **argv) {
    setbuf(stdout,NULL);log_event("MAIN");
    if(!dlopen("/System/Library/Frameworks/UIKit.framework/UIKit",RTLD_NOW)) {
        fprintf(stderr,"UIKit: %s\n",dlerror());return 1;
    }
    cls=dlsym(RTLD_DEFAULT,"objc_getClass");sel=dlsym(RTLD_DEFAULT,"sel_registerName");msg=dlsym(RTLD_DEFAULT,"objc_msgSend");
    Obj (*allocate)(Obj,const char*,size_t)=dlsym(RTLD_DEFAULT,"objc_allocateClassPair");
    void (*register_class)(Obj)=dlsym(RTLD_DEFAULT,"objc_registerClassPair");
    bool (*add_method)(Obj,Obj,void(*)(void),const char*)=dlsym(RTLD_DEFAULT,"class_addMethod");
    bool (*add_protocol)(Obj,Obj)=dlsym(RTLD_DEFAULT,"class_addProtocol");
    Obj (*protocol)(const char*)=dlsym(RTLD_DEFAULT,"objc_getProtocol");
    int (*app_main)(int,char**,Obj,Obj)=dlsym(RTLD_DEFAULT,"UIApplicationMain");
    if(!cls||!sel||!msg||!allocate||!register_class||!add_method||!add_protocol||!protocol||!app_main)return 2;
    Obj pool=get(get(cls("NSAutoreleasePool"),"alloc"),"init");(void)pool;
    Obj delegate=allocate(cls("NSObject"),"TouchAppDelegate",0);
    Obj scene=allocate(cls("NSObject"),"TouchSceneDelegate",0);
    Obj controller=allocate(cls("UIViewController"),"TouchViewController",0);
    if(!delegate||!scene||!controller)return 3;
    if(!add_protocol(delegate,protocol("UIApplicationDelegate"))||!add_protocol(scene,protocol("UIWindowSceneDelegate")))return 4;
    if(!add_method(delegate,sel("application:didFinishLaunchingWithOptions:"),(void(*)(void))launched,"B@:@@")||
       !add_method(scene,sel("scene:willConnectToSession:options:"),(void(*)(void))connect_scene,"v@:@@@")||
       !add_method(scene,sel("sceneDidBecomeActive:"),(void(*)(void))scene_active,"v@:@")||
       !add_method(scene,sel("tapped:"),(void(*)(void))tapped,"v@:@")||
       !add_method(controller,sel("viewDidLayoutSubviews"),(void(*)(void))layout,"v@:"))return 5;
    register_class(delegate);register_class(scene);register_class(controller);
    return app_main(argc,argv,NULL,str("TouchAppDelegate"));
}
