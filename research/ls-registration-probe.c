/* 24A437 LaunchServices application-record experiment.
 * registerApplication: at 0x186f85c70 calls LSRegisterURL(url, false).
 * Default mode only queries. Registration is explicit and limited to the
 * existing firmware SpringBoard or Setup bundle. --query accepts a bundle identifier
 * without modifying registration. Run a fresh process to verify registration.
 */
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <grp.h>
typedef void *Obj;
static void *message;
static void *(*selector)(const char *);
static Obj (*class_named)(const char *);
static Obj get(Obj o, const char *s) { return ((Obj(*)(Obj,void*))message)(o,selector(s)); }
static Obj arg(Obj o,const char *s,Obj a) { return ((Obj(*)(Obj,void*,Obj))message)(o,selector(s),a); }
static int responds(Obj o,const char *s) { return o && ((signed char(*)(Obj,void*,void*))message)(o,selector("respondsToSelector:"),selector(s)); }
static Obj string(const char *s) { return ((Obj(*)(Obj,void*,const char*))message)(class_named("NSString"),selector("stringWithUTF8String:"),s); }
static void print_object(const char *key,Obj o) {
    Obj d=get(o,"description");
    const char *s=((const char*(*)(Obj,void*))message)(d,selector("UTF8String"));
    printf("%s=%s\n",key,s?s:"(nil)");
}
int main(int argc,char **argv) {
    int rebuild=argc==2&&!strcmp(argv[1],"--rebuild-system");
    int setup_registration=argc==2&&!strcmp(argv[1],"--register-setup");
    int registration=(argc==2&&!strcmp(argv[1],"--register-springboard"))||setup_registration;
    int query=argc==3&&!strcmp(argv[1],"--query")&&argv[2][0];
    if(argc>1&&!registration&&!rebuild&&!query) { puts("usage: ls-registration-probe [--query BUNDLE_ID|--register-springboard|--register-setup|--rebuild-system]");return 2; }
    setbuf(stdout,NULL);alarm(rebuild?60:30);
    if(geteuid()==0 && (setgroups(0,NULL)||setgid(501)||setuid(501))) { perror("become mobile");return 1; }
    if(geteuid()!=501) { puts("MOBILE_IDENTITY_REQUIRED");return 1; }
    setenv("HOME","/var/mobile",1);setenv("CFFIXED_USER_HOME","/var/mobile",1);
    printf("LS_PROBE_BEGIN UID=%u GID=%u REGISTER=%d\n",getuid(),getgid(),registration);
    if(!dlopen("/System/Library/Frameworks/CoreServices.framework/CoreServices",RTLD_NOW)) { printf("DLOPEN_ERROR=%s\n",dlerror());return 1; }
    class_named=(Obj(*)(const char*))dlsym(RTLD_DEFAULT,"objc_getClass");
    selector=(void*(*)(const char*))dlsym(RTLD_DEFAULT,"sel_registerName");
    message=dlsym(RTLD_DEFAULT,"objc_msgSend");
    if(!class_named||!selector||!message) return 1;
    const char *bundle_id=query?argv[2]:(setup_registration?"com.apple.purplebuddy":"com.apple.springboard");
    Obj identifier=string(bundle_id);
    Obj proxy_class=class_named("LSApplicationProxy");
    if(!responds(proxy_class,"applicationProxyForIdentifier:")) { puts("PROXY_SELECTOR_MISSING");return 1; }
    if(!rebuild) {
        puts("LS_QUERY_BEGIN");
        printf("LS_IDENTIFIER=%s\n",bundle_id);
        Obj proxy=arg(proxy_class,"applicationProxyForIdentifier:",identifier);
        print_object("LS_PROXY",proxy);
        if(responds(proxy,"isInstalled")) printf("LS_IS_INSTALLED=%d\n",((signed char(*)(Obj,void*))message)(proxy,selector("isInstalled")));
        if(responds(proxy,"bundleURL")) print_object("LS_BUNDLE_URL",get(proxy,"bundleURL"));
    }
    if(rebuild) {
        Obj workspace=get(class_named("LSApplicationWorkspace"),"defaultWorkspace");
        const char *method="_LSPrivateRebuildApplicationDatabasesForSystemApps:internal:user:uid:";
        if(!responds(workspace,method)) { puts("REBUILD_SELECTOR_MISSING");return 1; }
        puts("LS_REBUILD_BEGIN SYSTEM=1 INTERNAL=0 USER=0 UID=501");
        unsigned int uid=501;
        int ok=((signed char(*)(Obj,void*,signed char,signed char,signed char,const unsigned int*))message)(workspace,selector(method),1,0,0,&uid);
        printf("LS_REBUILD_RESULT=%d\n",ok);
        if(!ok) return 1;
    }
    if(registration) {
        Obj workspace_class=class_named("LSApplicationWorkspace");
        if(!responds(workspace_class,"defaultWorkspace")) return 1;
        Obj workspace=get(workspace_class,"defaultWorkspace");
        if(!responds(workspace,"registerApplication:")) { puts("REGISTER_SELECTOR_MISSING");return 1; }
        const char *path=setup_registration?"/Applications/Setup.app":"/System/Library/CoreServices/SpringBoard.app";
        Obj url=arg(class_named("NSURL"),"fileURLWithPath:",string(path));
        puts("LS_REGISTER_BEGIN");
        printf("LS_REGISTER_PATH=%s\n",path);
        int ok=((signed char(*)(Obj,void*,Obj))message)(workspace,selector("registerApplication:"),url);
        printf("LS_REGISTER_RESULT=%d\n",ok);
        if(!ok) return 1;
    }
    puts("LS_PROBE_END");return 0;
}
