/* Guest-only diagnostic: dispatch returning does not prove UI delivery.
 * Apple IOHIDFamily's monitor tool uses the same event-system APIs and a
 * synthetic sender ID. Private ABI checked against 24A437 IOKit symbols.
 */
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static const uint64_t sender_id = UINT64_C(0xa190000000006500);
static uint64_t (*get_sender)(void *);
static unsigned (*get_type)(void *);
static long (*get_integer)(void *, unsigned);
static unsigned observed;

static void event_callback(void *target, void *refcon, void *sender, void *event) {
    (void)target; (void)refcon; (void)sender;
    if (get_sender(event) != sender_id) return;
    ++observed;
    printf("HID_OBSERVED TYPE=%u PAGE=%ld USAGE=%ld DOWN=%ld\n", get_type(event),
           get_integer(event, 0x30000), get_integer(event, 0x30001),
           get_integer(event, 0x30002));
}

int main(int argc, char **argv) {
    int home = argc == 2 && !strcmp(argv[1], "--home");
    if (argc > 1 && !home) { puts("usage: hid-input-probe [--home]"); return 2; }
    setbuf(stdout, NULL);
    alarm(25);
    puts("HID_PROBE_BEGIN");
    void *io = dlopen("/System/Library/Frameworks/IOKit.framework/IOKit", RTLD_NOW);
    void *cf = dlopen("/System/Library/Frameworks/CoreFoundation.framework/CoreFoundation", RTLD_NOW);
    if (!io || !cf) { printf("DLOPEN_ERROR=%s\n", dlerror()); return 1; }
#define LOAD(handle, name, result, ...) \
    result (*name)(__VA_ARGS__) = (result (*)(__VA_ARGS__))dlsym(handle, #name); \
    if (!name) { puts("MISSING " #name); return 1; }
    LOAD(io, IOHIDEventSystemClientCreateWithType, void *, void *, unsigned, void *);
    LOAD(io, IOHIDEventSystemClientGetTypeString, const char *, unsigned);
    LOAD(io, IOHIDEventSystemClientCopyServices, void *, void *);
    LOAD(io, IOHIDServiceClientGetRegistryID, uint64_t, void *);
    LOAD(io, IOHIDEventSystemClientRegisterEventCallback, void, void *, void *, void *, void *);
    LOAD(io, IOHIDEventSystemClientScheduleWithRunLoop, void, void *, void *, void *);
    LOAD(io, IOHIDEventSystemClientUnscheduleWithRunLoop, void, void *, void *, void *);
    LOAD(io, IOHIDEventCreateKeyboardEvent, void *, void *, uint64_t, unsigned, unsigned, unsigned char, unsigned);
    LOAD(io, IOHIDEventSetSenderID, void, void *, uint64_t);
    LOAD(io, IOHIDEventSystemClientDispatchEvent, void, void *, void *);
    LOAD(cf, CFArrayGetCount, long, void *);
    LOAD(cf, CFArrayGetValueAtIndex, void *, void *, long);
    LOAD(cf, CFRunLoopGetCurrent, void *, void);
    LOAD(cf, CFRunLoopRunInMode, int, void *, double, unsigned char);
    LOAD(cf, CFShow, void, void *);
    LOAD(cf, CFRelease, void, void *);
    LOAD(RTLD_DEFAULT, mach_absolute_time, uint64_t, void);
    get_sender = (uint64_t (*)(void *))dlsym(io, "IOHIDEventGetSenderID");
    get_type = (unsigned (*)(void *))dlsym(io, "IOHIDEventGetType");
    get_integer = (long (*)(void *, unsigned))dlsym(io, "IOHIDEventGetIntegerValue");
    void **mode = dlsym(cf, "kCFRunLoopDefaultMode");
    if (!get_sender || !get_type || !get_integer || !mode) return 1;
    printf("HID_CLIENT_TYPE=1 NAME=%s\n", IOHIDEventSystemClientGetTypeString(1));
    void *client = IOHIDEventSystemClientCreateWithType(NULL, 1, NULL);
    printf("HID_CLIENT=%d\n", client != NULL);
    if (!client) return 1;
    void *loop = CFRunLoopGetCurrent();
    IOHIDEventSystemClientRegisterEventCallback(client, (void *)event_callback, NULL, NULL);
    IOHIDEventSystemClientScheduleWithRunLoop(client, loop, *mode);
    (void)CFRunLoopRunInMode(*mode, 0.2, 0);
    void *services = IOHIDEventSystemClientCopyServices(client);
    long count = services ? CFArrayGetCount(services) : -1;
    printf("HID_SERVICES=%ld\n", count);
    if (services) {
        for (long i = 0; i < count; ++i) {
            void *service = CFArrayGetValueAtIndex(services, i);
            printf("HID_SERVICE_INDEX=%ld REGISTRY_ID=0x%llx\n", i,
                   (unsigned long long)IOHIDServiceClientGetRegistryID(service));
            CFShow(service);
        }
        CFRelease(services);
    }
    if (home) {
        /* Consumer page, Menu usage. Always send the matching release. */
        for (unsigned down = 1; ; down = 0) {
            void *event = IOHIDEventCreateKeyboardEvent(NULL, mach_absolute_time(), 0x0c, 0x40, down, 0);
            if (!event) return 1;
            IOHIDEventSetSenderID(event, sender_id);
            printf("HID_DISPATCH_BEGIN PAGE=12 USAGE=64 DOWN=%u\n", down);
            IOHIDEventSystemClientDispatchEvent(client, event);
            puts("HID_DISPATCH_RETURNED");
            CFRelease(event);
            (void)CFRunLoopRunInMode(*mode, 0.15, 0);
            if (!down) break;
        }
        (void)CFRunLoopRunInMode(*mode, 2.0, 0);
    }
    printf("HID_MATCHING_CALLBACKS=%u\n", observed);
    IOHIDEventSystemClientUnscheduleWithRunLoop(client, loop, *mode);
    CFRelease(client);
    puts("HID_PROBE_END");
    return 0;
}
