/* Query the real guest QuartzCore display API without requiring iPhone headers.
 * Uses runtime discovery of the exact image's CADisplay class/selectors.
 */
extern void *dlopen(const char *, int), *dlsym(void *, const char *);
extern const char *dlerror(void);
extern int printf(const char *, ...), fflush(void *);
typedef void *Obj;
typedef Obj (*ObjectMessage)(Obj, void *);
typedef unsigned long (*CountMessage)(Obj, void *);
typedef const char *(*StringMessage)(Obj, void *);
int main(void) {
    printf("DISPLAY_PROBE_BEGIN\n"); fflush(0);
    void *handle = dlopen("/System/Library/Frameworks/QuartzCore.framework/QuartzCore", 1);
    if (!handle) { printf("DLOPEN_ERROR=%s\n", dlerror()); return 1; }
    Obj (*get_class)(const char *) = (Obj (*)(const char *))dlsym((void *)-2, "objc_getClass");
    void *(*selector)(const char *) = (void *(*)(const char *))dlsym((void *)-2, "sel_registerName");
    void *message = dlsym((void *)-2, "objc_msgSend");
    if (!get_class || !selector || !message) { printf("RUNTIME_SYMBOLS_MISSING\n"); return 1; }
    Obj cls = get_class("CADisplay");
    if (!cls) { printf("CADISPLAY_CLASS_MISSING\n"); return 1; }
    printf("CADISPLAY_QUERY_BEGIN\n"); fflush(0);
    Obj displays = ((ObjectMessage)message)(cls, selector("displays"));
    unsigned long count = ((CountMessage)message)(displays, selector("count"));
    printf("CADISPLAY_COUNT=%lu\n", count); fflush(0);
    Obj main_display = ((ObjectMessage)message)(cls, selector("mainDisplay"));
    printf("CADISPLAY_MAIN_PRESENT=%d\n", main_display != 0); fflush(0);
    if (main_display) {
        Obj name = ((ObjectMessage)message)(main_display, selector("name"));
        const char *text = ((StringMessage)message)(name, selector("UTF8String"));
        printf("CADISPLAY_MAIN_NAME=%s\n", text ? text : "(nil)");
    }
    printf("DISPLAY_PROBE_END\n");
    return 0;
}
