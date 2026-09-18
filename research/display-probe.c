/* Query the real guest QuartzCore display API without requiring iPhone headers.
 * Uses runtime discovery of the exact image's CADisplay class/selectors.
 */
#include <dlfcn.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <mach/mach.h>
extern int sample_task_threads(task_t);
static void *sample_self(void *ignored) {
    (void)ignored;
    sleep(5);
    printf("DISPLAY_SELF_SAMPLE_BEGIN\n"); fflush(stdout);
    sample_task_threads(mach_task_self());
    printf("DISPLAY_SELF_SAMPLE_END\n"); fflush(stdout);
    return 0;
}
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
    pthread_t sampler;
    int error = pthread_create(&sampler, 0, sample_self, 0);
    printf("SAMPLER_CREATE_RESULT=%d\n", error); fflush(stdout);
    if (!error) pthread_detach(sampler);
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
