/* Offline UIKit notebook. Uses runtime Objective-C calls with the arm64 ABI. */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <dlfcn.h>

typedef void *Obj;
typedef struct { double x,y; } Point;
typedef struct { double w,h; } Size;
typedef struct { double x,y,w,h; } Rect;
typedef struct { float x,y; uint32_t start; } Ink;
_Static_assert(sizeof(Ink)==12,"ink storage format");
static Obj (*cls)(const char *),(*sel)(const char *);
static void *msg;
static Obj window,scroll,editor,canvas,status,heading,picture,save_button,mode_button;
static Obj path;
static Ink ink[8192];
static size_t count;
static bool drawing;
static Obj get(Obj o,const char*s){return ((Obj(*)(Obj,Obj))msg)(o,sel(s));}
static Obj arg(Obj o,const char*s,Obj a){return ((Obj(*)(Obj,Obj,Obj))msg)(o,sel(s),a);}
static void set(Obj o,const char*s,Obj a){((void(*)(Obj,Obj,Obj))msg)(o,sel(s),a);}
static void flag(Obj o,const char*s,bool b){((void(*)(Obj,Obj,bool))msg)(o,sel(s),b);}
static void call(Obj o,const char*s){((void(*)(Obj,Obj))msg)(o,sel(s));}
static Obj str(const char*s){return ((Obj(*)(Obj,Obj,const char*))msg)(cls("NSString"),sel("stringWithUTF8String:"),s);}
static Obj make(const char*s){return get(get(cls(s),"alloc"),"init");}
static void frame(Obj o,Rect r){((void(*)(Obj,Obj,Rect))msg)(o,sel("setFrame:"),r);}
static Rect bounds(Obj o){return ((Rect(*)(Obj,Obj))msg)(o,sel("bounds"));}
static bool kind(Obj o,const char*s){return ((bool(*)(Obj,Obj,Obj))msg)(o,sel("isKindOfClass:"),cls(s));}
static void log_event(const char*s){printf("NOTEBOOK %s\n",s);}
static void message(const char*s){set(status,"setText:",str(s));log_event(s);}
static void store(void){
    if(!path || !editor){message("Save unavailable");return;}
    Obj dict=make("NSMutableDictionary");
    set(dict,"setDictionary:",get(cls("NSDictionary"),"dictionary"));
    Obj note=get(editor,"text");
    Obj data=((Obj(*)(Obj,Obj,const void*,unsigned long))msg)(cls("NSData"),sel("dataWithBytes:length:"),ink,count*sizeof(Ink));
    Obj version=((Obj(*)(Obj,Obj,int))msg)(cls("NSNumber"),sel("numberWithInt:"),1);
    const char*keys[]={"note","ink-v1","version"}; Obj values[]={note,data,version};
    for(int i=0;i<3;i++)((void(*)(Obj,Obj,Obj,Obj))msg)(dict,sel("setObject:forKey:"),values[i],str(keys[i]));
    bool ok=((bool(*)(Obj,Obj,Obj,bool))msg)(dict,sel("writeToFile:atomically:"),path,true);
    call(dict,"release");message(ok?"Saved on this iPhone":"Save failed");
}
static void save(Obj self,Obj cmd,Obj sender){(void)self;(void)cmd;(void)sender;flag(window,"endEditing:",true);store();}
static void inactive(Obj self,Obj cmd,Obj scene){(void)self;(void)cmd;(void)scene;store();}
static void edited(Obj self,Obj cmd,Obj text){(void)self;(void)cmd;(void)text;message("Unsaved changes");}
static void toggle(Obj self,Obj cmd,Obj sender){
    (void)self;(void)cmd;(void)sender;drawing=!drawing;
    flag(window,"endEditing:",true);flag(scroll,"setScrollEnabled:",!drawing);
    message(drawing?"Draw on the canvas; tap Draw / Scroll to scroll":"Scroll mode");
}
static void paint(Obj self,Obj cmd,Rect rect){
    (void)cmd;(void)rect;Rect b=bounds(self);Obj line=get(cls("UIBezierPath"),"bezierPath");
    call(get(cls("UIColor"),"blackColor"),"setStroke");
    ((void(*)(Obj,Obj,double))msg)(line,sel("setLineWidth:"),3.0);
    for(size_t i=0;i<count;i++){
        Point p={ink[i].x*b.w,ink[i].y*b.h};
        ((void(*)(Obj,Obj,Point))msg)(line,sel(ink[i].start?"moveToPoint:":"addLineToPoint:"),p);
    }
    call(line,"stroke");
}
static void append_touch(Obj self,Obj touches,bool start){
    if(!drawing || count==8192)return;
    Obj touch=get(touches,"anyObject"); if(!touch)return;
    Point p=((Point(*)(Obj,Obj,Obj))msg)(touch,sel("locationInView:"),self);Rect b=bounds(self);
    if(b.w<=0||b.h<=0||!isfinite(p.x)||!isfinite(p.y))return;
    double x=p.x/b.w,y=p.y/b.h;x=x<0?0:x>1?1:x;y=y<0?0:y>1?1:y;
    ink[count]=(Ink){(float)x,(float)y,start||count==0};
    count++;
    call(self,"setNeedsDisplay");message("Unsaved drawing");
}
static void began(Obj self,Obj cmd,Obj touches,Obj event){(void)cmd;(void)event;append_touch(self,touches,true);}
static void moved(Obj self,Obj cmd,Obj touches,Obj event){(void)cmd;(void)event;append_touch(self,touches,false);}
static void ended(Obj self,Obj cmd,Obj touches,Obj event){(void)cmd;(void)event;append_touch(self,touches,false);if(drawing)store();}
static void cancelled(Obj self,Obj cmd,Obj touches,Obj event){(void)self;(void)cmd;(void)touches;(void)event;if(drawing)store();}
static Obj label(const char*text,double size){
    Obj o=make("UILabel");set(o,"setText:",str(text));set(o,"setTextColor:",get(cls("UIColor"),"blackColor"));
    set(o,"setFont:",((Obj(*)(Obj,Obj,double))msg)(cls("UIFont"),sel("systemFontOfSize:"),size));return o;
}
static Obj button(Obj target,const char*title,const char*action){
    Obj o=((Obj(*)(Obj,Obj,long))msg)(cls("UIButton"),sel("buttonWithType:"),0);
    set(o,"setBackgroundColor:",get(cls("UIColor"),"lightGrayColor"));
    Obj text=label(title,16);frame(text,(Rect){8,4,160,36});flag(text,"setUserInteractionEnabled:",false);set(o,"addSubview:",text);
    ((void(*)(Obj,Obj,Obj,Obj,unsigned long))msg)(o,sel("addTarget:action:forControlEvents:"),target,sel(action),1UL<<6);return o;
}
static void layout(Obj self,Obj cmd){
    (void)cmd;Rect b=bounds(get(self,"view"));double w=b.w;
    frame(heading,(Rect){20,48,w-40,44});frame(save_button,(Rect){20,96,100,44});frame(mode_button,(Rect){132,96,w-152,44});
    frame(status,(Rect){20,145,w-40,40});frame(scroll,(Rect){0,190,w,b.h>210?b.h-210:1});
    frame(editor,(Rect){20,0,w-40,180});frame(picture,(Rect){20,205,w-40,210});frame(canvas,(Rect){20,440,w-40,400});
    ((void(*)(Obj,Obj,Size))msg)(scroll,sel("setContentSize:"),(Size){w,880});
}
static bool launched(Obj self,Obj cmd,Obj app,Obj opts){(void)self;(void)cmd;(void)app;(void)opts;log_event("LAUNCHED");return true;}
static void connect_scene(Obj self,Obj cmd,Obj scene,Obj session,Obj opts){
    (void)cmd;(void)session;(void)opts;
    window=arg(get(cls("UIWindow"),"alloc"),"initWithWindowScene:",scene);Obj controller=make("NotebookController");Obj view=get(controller,"view");
    set(view,"setBackgroundColor:",get(cls("UIColor"),"whiteColor"));heading=label("Field Notes",30);status=label("Write, scroll, and sketch offline",12);
    save_button=button(self,"Save","save:");mode_button=button(self,"Draw / Scroll","toggle:");scroll=make("UIScrollView");
    editor=make("UITextView");set(editor,"setText:",str("A note from my virtual iPhone."));set(editor,"setDelegate:",self);
    set(editor,"setTextColor:",get(cls("UIColor"),"blackColor"));set(editor,"setBackgroundColor:",get(cls("UIColor"),"whiteColor"));
    set(editor,"setFont:",((Obj(*)(Obj,Obj,double))msg)(cls("UIFont"),sel("systemFontOfSize:"),22.0));
    Obj bundle=get(cls("NSBundle"),"mainBundle");Obj photo=((Obj(*)(Obj,Obj,Obj,Obj))msg)(bundle,sel("pathForResource:ofType:"),str("Photo"),str("png"));
    Obj image=arg(cls("UIImage"),"imageWithContentsOfFile:",photo);picture=arg(get(cls("UIImageView"),"alloc"),"initWithImage:",image);
    ((void(*)(Obj,Obj,long))msg)(picture,sel("setContentMode:"),1);canvas=make("NotebookCanvas");set(canvas,"setBackgroundColor:",get(cls("UIColor"),"lightGrayColor"));
    Obj (*dirs)(unsigned long,unsigned long,bool)=dlsym(RTLD_DEFAULT,"NSSearchPathForDirectoriesInDomains");
    Obj directory=dirs?get(dirs(9,1,true),"firstObject"):NULL;
    if(directory){
        Obj manager=get(cls("NSFileManager"),"defaultManager");Obj error=NULL;
        bool ok=((bool(*)(Obj,Obj,Obj,bool,Obj,Obj*))msg)(manager,sel("createDirectoryAtPath:withIntermediateDirectories:attributes:error:"),directory,true,NULL,&error);
        if(ok)path=get(arg(directory,"stringByAppendingPathComponent:",str("Notebook.plist")),"copy");
    }
    Obj saved=path?arg(cls("NSDictionary"),"dictionaryWithContentsOfFile:",path):NULL;
    if(saved){
        Obj text=arg(saved,"objectForKey:",str("note")),data=arg(saved,"objectForKey:",str("ink-v1"));
        long version=((long(*)(Obj,Obj))msg)(arg(saved,"objectForKey:",str("version")),sel("integerValue"));
        if(version==1 && kind(text,"NSString") && kind(data,"NSData")){
            unsigned long n=((unsigned long(*)(Obj,Obj))msg)(data,sel("length"));
            if(n<=sizeof(ink)&&n%sizeof(Ink)==0){
                if(n)memcpy(ink,get(data,"bytes"),n);count=n/sizeof(Ink);
                for(size_t i=0;i<count;i++)if(!isfinite(ink[i].x)||!isfinite(ink[i].y)||ink[i].x<0||ink[i].x>1||ink[i].y<0||ink[i].y>1||ink[i].start>1){count=0;break;}
                if(count)ink[0].start=1;set(editor,"setText:",text);message("Loaded saved note");
            }
        }
    }
    Obj children[]={heading,save_button,mode_button,status,scroll};for(int i=0;i<5;i++)set(view,"addSubview:",children[i]);
    set(scroll,"addSubview:",editor);set(scroll,"addSubview:",picture);set(scroll,"addSubview:",canvas);
    set(window,"setRootViewController:",controller);call(window,"makeKeyAndVisible");log_event(image?"SCENE_VISIBLE_WITH_IMAGE":"SCENE_VISIBLE_IMAGE_MISSING");
}
int main(int argc,char**argv){
    setbuf(stdout,NULL);if(!dlopen("/System/Library/Frameworks/UIKit.framework/UIKit",RTLD_NOW))return 1;
    cls=dlsym(RTLD_DEFAULT,"objc_getClass");sel=dlsym(RTLD_DEFAULT,"sel_registerName");msg=dlsym(RTLD_DEFAULT,"objc_msgSend");
    Obj(*allocate)(Obj,const char*,size_t)=dlsym(RTLD_DEFAULT,"objc_allocateClassPair");void(*reg)(Obj)=dlsym(RTLD_DEFAULT,"objc_registerClassPair");
    bool(*method)(Obj,Obj,void(*)(void),const char*)=dlsym(RTLD_DEFAULT,"class_addMethod");int(*app)(int,char**,Obj,Obj)=dlsym(RTLD_DEFAULT,"UIApplicationMain");
    Obj(*protocol)(const char*)=dlsym(RTLD_DEFAULT,"objc_getProtocol");
    bool(*adopt)(Obj,Obj)=dlsym(RTLD_DEFAULT,"class_addProtocol");
    if(!cls||!sel||!msg||!allocate||!reg||!method||!app||!protocol||!adopt)return 2;
    Obj pool=make("NSAutoreleasePool");(void)pool;
    Obj delegate=allocate(cls("NSObject"),"NotebookAppDelegate",0),scene=allocate(cls("NSObject"),"NotebookSceneDelegate",0),controller=allocate(cls("UIViewController"),"NotebookController",0),draw=allocate(cls("UIView"),"NotebookCanvas",0);
    if(!delegate||!scene||!controller||!draw)return 3;
    if(!adopt(delegate,protocol("UIApplicationDelegate")) || !adopt(scene,protocol("UIWindowSceneDelegate")) || !adopt(scene,protocol("UITextViewDelegate")))return 3;
#define ADD(c,s,f,t) if(!method(c,sel(s),(void(*)(void))f,t))return 4
    ADD(delegate,"application:didFinishLaunchingWithOptions:",launched,"B@:@@");
    ADD(scene,"scene:willConnectToSession:options:",connect_scene,"v@:@@@");ADD(scene,"sceneWillResignActive:",inactive,"v@:@");
    ADD(scene,"save:",save,"v@:@");ADD(scene,"toggle:",toggle,"v@:@");ADD(scene,"textViewDidChange:",edited,"v@:@");
    ADD(controller,"viewDidLayoutSubviews",layout,"v@:");ADD(draw,"drawRect:",paint,"v@:{CGRect={CGPoint=dd}{CGSize=dd}}");
    ADD(draw,"touchesBegan:withEvent:",began,"v@:@@");ADD(draw,"touchesMoved:withEvent:",moved,"v@:@@");ADD(draw,"touchesEnded:withEvent:",ended,"v@:@@");ADD(draw,"touchesCancelled:withEvent:",cancelled,"v@:@@");
    reg(delegate);reg(scene);reg(controller);reg(draw);return app(argc,argv,NULL,str("NotebookAppDelegate"));
}
