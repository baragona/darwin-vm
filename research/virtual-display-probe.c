/* Isolated construction/empty-render experiment for iOS 27 / 24A437.
 * Does not register a render service, attach to backboardd, or alter its displays.
 * A returned object is not proof of framebuffer allocation or rendered output.
 */
#include <dlfcn.h>
#include <mach/mach.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef void *Obj;
typedef struct { double x, y, width, height; } Rect;
extern int sample_task_threads(task_t);
static void *sample_self(void *unused) {
    (void)unused;
    sleep(5);
    puts("VIRTUAL_SELF_SAMPLE_BEGIN"); fflush(stdout);
    sample_task_threads(mach_task_self());
    puts("VIRTUAL_SELF_SAMPLE_END"); fflush(stdout);
    return 0;
}
int main(int argc, char **argv) {
    int render_empty = argc == 2 && !strcmp(argv[1], "--render-empty");
    if (argc > 1 && !render_empty) {
        puts("usage: virtual-display-probe [--render-empty]"); return 2;
    }
    setbuf(stdout, NULL);
    puts("VIRTUAL_PROBE_BEGIN");
    alarm(20); /* Bound only this diagnostic process, not the VM. */
    void *handle = dlopen("/System/Library/Frameworks/QuartzCore.framework/QuartzCore", RTLD_NOW);
    if (!handle) { printf("DLOPEN_ERROR=%s\n", dlerror()); return 1; }
    Obj (*cls)(const char *) = (Obj (*)(const char *))dlsym(RTLD_DEFAULT, "objc_getClass");
    void *(*sel)(const char *) = (void *(*)(const char *))dlsym(RTLD_DEFAULT, "sel_registerName");
    void *msg = dlsym(RTLD_DEFAULT, "objc_msgSend");
    if (!cls || !sel || !msg) { puts("RUNTIME_SYMBOLS_MISSING"); return 1; }
    Obj display_class = cls("CAWindowServerVirtualDisplay");
    if (!display_class) { puts("VIRTUAL_CLASS_MISSING"); return 1; }
    Obj (*get)(Obj, void *) = (Obj (*)(Obj, void *))msg;
    Obj (*arg)(Obj, void *, Obj) = (Obj (*)(Obj, void *, Obj))msg;
    Obj options = get(cls("NSMutableDictionary"), sel("dictionary"));
    const char *keys[] = {"kCAVirtualDisplayWidth", "kCAVirtualDisplayHeight", "kCAVirtualDisplayUpdateRate"};
    const double values[] = {1024, 768, 60};
    for (unsigned i = 0; i < 3; ++i) {
        Obj key = ((Obj (*)(Obj, void *, const char *))msg)(cls("NSString"), sel("stringWithUTF8String:"), keys[i]);
        Obj value = ((Obj (*)(Obj, void *, double))msg)(cls("NSNumber"), sel("numberWithDouble:"), values[i]);
        ((void (*)(Obj, void *, Obj, Obj))msg)(options, sel("setObject:forKey:"), value, key);
    }
    pthread_t sampler;
    int error = pthread_create(&sampler, NULL, sample_self, NULL);
    printf("SAMPLER_CREATE_RESULT=%d\n", error);
    if (!error) pthread_detach(sampler);
    puts("VIRTUAL_ALLOC_BEGIN");
    Obj allocated = get(display_class, sel("alloc"));
    printf("VIRTUAL_ALLOC_PRESENT=%d\n", allocated != NULL);
    puts("VIRTUAL_INIT_BEGIN");
    Obj display = arg(allocated, sel("initWithOptions:"), options);
    printf("VIRTUAL_INIT_PRESENT=%d\n", display != NULL);
    if (!display) return 1;
    Obj name = get(display, sel("name"));
    const char *text = ((const char *(*)(Obj, void *))msg)(name, sel("UTF8String"));
    printf("VIRTUAL_NAME=%s\n", text ? text : "(nil)");
    printf("VIRTUAL_ID=%u\n", ((unsigned int (*)(Obj, void *))msg)(display, sel("displayId")));
    Rect bounds = ((Rect (*)(Obj, void *))msg)(display, sel("bounds"));
    printf("VIRTUAL_BOUNDS=%g,%g,%g,%g\n", bounds.x, bounds.y, bounds.width, bounds.height);
    void *(*method)(Obj, void *) = (void *(*)(Obj, void *))dlsym(RTLD_DEFAULT, "class_getInstanceMethod");
    const char *(*encoding)(void *) = (const char *(*)(void *))dlsym(RTLD_DEFAULT, "method_getTypeEncoding");
    char *(*return_type)(void *) = (char *(*)(void *))dlsym(RTLD_DEFAULT, "method_copyReturnType");
    char *(*argument_type)(void *, unsigned) = (char *(*)(void *, unsigned))dlsym(RTLD_DEFAULT, "method_copyArgumentType");
    if (!method || !encoding || !return_type || !argument_type) return 1;
    const char *inspect[] = {"renderForTime:", "acquireFrozenSurface", "beginExternalUpdate:usingSoftwareRenderer:"};
    for (unsigned i = 0; i < 3; ++i) {
        void *m = method(display_class, sel(inspect[i]));
        printf("VIRTUAL_METHOD=%s TYPE=%s\n", inspect[i], m ? encoding(m) : "missing");
    }
    if (render_empty) {
        void *m = method(display_class, sel("renderForTime:"));
        char *ret = m ? return_type(m) : NULL;
        char *time_arg = m ? argument_type(m, 2) : NULL;
        int compatible = ret && time_arg && !strcmp(ret, "v") && !strcmp(time_arg, "d");
        free(ret); free(time_arg);
        if (!compatible) { puts("VIRTUAL_RENDER_SIGNATURE_UNSUPPORTED"); return 1; }
        puts("VIRTUAL_RENDER_EMPTY_BEGIN");
        ((void (*)(Obj, void *, double))msg)(display, sel("renderForTime:"), 1.0);
        puts("VIRTUAL_RENDER_EMPTY_RETURNED");
        m = method(display_class, sel("acquireFrozenSurface"));
        ret = m ? return_type(m) : NULL;
        if (ret && ret[0] == '^') {
            void *surface = get(display, sel("acquireFrozenSurface"));
            printf("VIRTUAL_FROZEN_SURFACE_PRESENT=%d\n", surface != NULL);
            /* Opaque pointer only: do not assume this is an IOSurface. */
        }
        free(ret);
    }
    puts("VIRTUAL_PROBE_END");
    /* Process exit owns cleanup; destruction of an unattached private object is
     * intentionally outside this bounded experiment. */
    return 0;
}
