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
static double (*get_float)(void *, unsigned);
static void *(*get_children)(void *);
static long (*array_count)(void *);
static void *(*array_value)(void *, long);
static unsigned observed;

static void event_callback(void *target, void *refcon, void *sender, void *event) {
    (void)target; (void)refcon; (void)sender;
    if (get_sender(event) != sender_id) return;
    ++observed;
    unsigned type = get_type(event);
    if (type == 11) {
        void *children = get_children(event);
        long count = children ? array_count(children) : 0;
        printf("HID_OBSERVED TYPE=11 MASK=%ld TOUCH=%ld INTEGRATED=%ld X=%.3f Y=%.3f CHILDREN=%ld\n",
               get_integer(event, 0xb0007), get_integer(event, 0xb0009),
               get_integer(event, 0xb0019), get_float(event, 0xb0000),
               get_float(event, 0xb0001), count);
        if (count == 1) {
            void *finger = array_value(children, 0);
            printf("HID_FINGER TOUCH=%ld X=%.3f Y=%.3f\n",
                   get_integer(finger, 0xb0009), get_float(finger, 0xb0000),
                   get_float(finger, 0xb0001));
        }
        return;
    }
    printf("HID_OBSERVED TYPE=%u PAGE=%ld USAGE=%ld DOWN=%ld\n", type,
           get_integer(event, 0x30000), get_integer(event, 0x30001),
           get_integer(event, 0x30002));
}

int main(int argc, char **argv) {
    int home = argc == 2 && !strcmp(argv[1], "--home");
    int swipe = argc == 2 && !strcmp(argv[1], "--swipe");
    if (argc > 1 && !home && !swipe) { puts("usage: hid-input-probe [--home | --swipe]"); return 2; }
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
    LOAD(io, IOHIDServiceClientGetRegistryID, void *, void *);
    LOAD(io, IOHIDEventSystemClientRegisterEventCallback, void, void *, void *, void *, void *);
    LOAD(io, IOHIDEventSystemClientScheduleWithRunLoop, void, void *, void *, void *);
    LOAD(io, IOHIDEventSystemClientUnscheduleWithRunLoop, void, void *, void *, void *);
    LOAD(io, IOHIDEventCreateKeyboardEvent, void *, void *, uint64_t, unsigned, unsigned, unsigned char, unsigned);
    LOAD(io, IOHIDEventSetSenderID, void, void *, uint64_t);
    LOAD(io, IOHIDEventSetTimeStamp, void, void *, uint64_t);
    LOAD(io, IOHIDEventSetIntegerValue, void, void *, unsigned, long);
    LOAD(io, IOHIDEventAppendEvent, void, void *, void *, unsigned);
    /* 24A437: doubles occupy d0-d4; range/touch/options use integer args.
     * Finger wrapper at 0x18f0a15ac forwards type=2 to the full constructor. */
    LOAD(io, IOHIDEventCreateDigitizerEvent, void *, void *, uint64_t,
         unsigned, unsigned, unsigned, unsigned, unsigned,
         double, double, double, double, double, unsigned, unsigned, unsigned);
    LOAD(io, IOHIDEventCreateDigitizerFingerEvent, void *, void *, uint64_t,
         unsigned, unsigned, unsigned, double, double, double, double, double,
         unsigned, unsigned, unsigned);
    LOAD(io, IOHIDEventSystemClientDispatchEvent, void, void *, void *);
    LOAD(cf, CFArrayGetCount, long, void *);
    LOAD(cf, CFArrayGetValueAtIndex, void *, void *, long);
    LOAD(cf, CFNumberGetValue, unsigned char, void *, int, void *);
    LOAD(cf, CFRunLoopGetCurrent, void *, void);
    LOAD(cf, CFRunLoopRunInMode, int, void *, double, unsigned char);
    LOAD(cf, CFShow, void, void *);
    LOAD(cf, CFRelease, void, void *);
    LOAD(RTLD_DEFAULT, mach_absolute_time, uint64_t, void);
    get_sender = (uint64_t (*)(void *))dlsym(io, "IOHIDEventGetSenderID");
    get_type = (unsigned (*)(void *))dlsym(io, "IOHIDEventGetType");
    get_integer = (long (*)(void *, unsigned))dlsym(io, "IOHIDEventGetIntegerValue");
    get_float = (double (*)(void *, unsigned))dlsym(io, "IOHIDEventGetFloatValue");
    get_children = (void *(*)(void *))dlsym(io, "IOHIDEventGetChildren");
    array_count = CFArrayGetCount;
    array_value = CFArrayGetValueAtIndex;
    void **mode = dlsym(cf, "kCFRunLoopDefaultMode");
    if (!get_sender || !get_type || !get_integer || !get_float || !get_children || !mode) return 1;
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
            void *number = IOHIDServiceClientGetRegistryID(service);
            int64_t registry_id = 0;
            /* Registry ID is a borrowed CFNumber, not an integer return. */
            int valid = number && CFNumberGetValue(number, 4, &registry_id);
            printf("HID_SERVICE_INDEX=%ld REGISTRY_ID_VALID=%d REGISTRY_ID=0x%llx\n",
                   i, valid, (unsigned long long)registry_id);
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
    if (swipe) {
        enum { MOVES = 12, FRAMES = MOVES + 2 };
        void *hands[FRAMES] = {0}, *fingers[FRAMES] = {0};
        /* Preallocate the entire sequence, including release, before sending
         * any touch. No input is sent if a constructor fails. */
        for (unsigned i = 0; i < FRAMES; ++i) {
            unsigned touch = i != FRAMES - 1;
            unsigned mask = i == 0 || !touch ? 3 : 4;
            double y = 0.96 - 0.76 * (i > MOVES ? MOVES : i) / MOVES;
            hands[i] = IOHIDEventCreateDigitizerEvent(NULL, 0, 3, 0, 0, mask, 0,
                0.5, y, 0, 0, 0, touch, touch, 0);
            fingers[i] = IOHIDEventCreateDigitizerFingerEvent(NULL, 0, 1, 1, mask,
                0.5, y, 0, touch ? 1.0 : 0.0, 0, touch, touch, 0);
            if (!hands[i] || !fingers[i]) {
                for (unsigned j = 0; j <= i; ++j) {
                    if (hands[j]) CFRelease(hands[j]);
                    if (fingers[j]) CFRelease(fingers[j]);
                }
                puts("HID_SWIPE_ALLOCATION_FAILED");
                return 1;
            }
            /* HID.framework's exact-build setter uses field 0xb0019. */
            IOHIDEventSetIntegerValue(hands[i], 0xb0019, 1);
            IOHIDEventSetIntegerValue(fingers[i], 0xb0019, 1);
            IOHIDEventSetSenderID(hands[i], sender_id);
            IOHIDEventSetSenderID(fingers[i], sender_id);
            IOHIDEventAppendEvent(hands[i], fingers[i], 0);
        }
        for (unsigned i = 0; i < FRAMES; ++i) {
            uint64_t now = mach_absolute_time();
            IOHIDEventSetTimeStamp(hands[i], now);
            IOHIDEventSetTimeStamp(fingers[i], now);
            printf("HID_SWIPE_FRAME=%u TOUCH=%ld Y=%.3f\n", i,
                   get_integer(fingers[i], 0xb0009), get_float(fingers[i], 0xb0001));
            IOHIDEventSystemClientDispatchEvent(client, hands[i]);
            /* Keep gesture duration independent of early run-loop returns. */
            usleep(40000);
            (void)CFRunLoopRunInMode(*mode, 0.001, 0);
        }
        (void)CFRunLoopRunInMode(*mode, 2.0, 0);
        for (unsigned i = 0; i < FRAMES; ++i) {
            CFRelease(hands[i]);
            CFRelease(fingers[i]);
        }
    }
    printf("HID_MATCHING_CALLBACKS=%u\n", observed);
    IOHIDEventSystemClientUnscheduleWithRunLoop(client, loop, *mode);
    CFRelease(client);
    puts("HID_PROBE_END");
    return 0;
}
