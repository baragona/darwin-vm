/* Guest-only HIDVirtualEventService delegate. Uses the exact framework's
 * Objective-C API without requiring a private SDK at build time.
 * Property callbacks run on one serial queue; only readiness is shared.
 */
#include "virtual-touch-service.h"
#include <dlfcn.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

static void *message;
static void *(*selector)(const char *);
static void *(*class_named)(const char *);
static void *properties, *service, *delegate;
static _Atomic int state;

static void *get(void *object, const char *name) {
    return ((void *(*)(void *, void *))message)(object, selector(name));
}
static void call(void *object, const char *name) {
    ((void (*)(void *, void *))message)(object, selector(name));
}
static void set(void *object, const char *name, void *value) {
    ((void (*)(void *, void *, void *))message)(object, selector(name), value);
}
static void *string(const char *text) {
    return ((void *(*)(void *, void *, const char *))message)(
        class_named("NSString"), selector("stringWithUTF8String:"), text);
}
static void *number(unsigned value) {
    return ((void *(*)(void *, void *, unsigned))message)(
        class_named("NSNumber"), selector("numberWithUnsignedInt:"), value);
}
static void put(void *dict, void *key, void *value) {
    ((void (*)(void *, void *, void *, void *))message)(
        dict, selector("setObject:forKey:"), value, key);
}
static const char *text(void *key) {
    const char *value = ((const char *(*)(void *, void *))message)(key, selector("UTF8String"));
    return value ? value : "(nil)";
}
static void *property(void *self, void *cmd, void *key, void *which) {
    (void)self; (void)cmd; (void)which;
    void *value = ((void *(*)(void *, void *, void *))message)(
        properties, selector("objectForKey:"), key);
    printf("VTOUCH_PROPERTY KEY=%s FOUND=%d\n", text(key), value != NULL);
    return value;
}
static unsigned char set_property(void *self, void *cmd, void *value, void *key, void *which) {
    (void)self; (void)cmd; (void)which;
    printf("VTOUCH_SET_PROPERTY KEY=%s PRESENT=%d\n", text(key), value != NULL);
    if (value) put(properties, key, value);
    else set(properties, "removeObjectForKey:", key);
    return 1;
}
static void *copy_event(void *self, void *cmd, void *matching, void *which) {
    (void)self; (void)cmd; (void)matching; (void)which;
    puts("VTOUCH_COPY_EVENT_UNAVAILABLE");
    return NULL;
}
static unsigned char output_event(void *self, void *cmd, void *event, void *which) {
    (void)self; (void)cmd; (void)event; (void)which;
    puts("VTOUCH_OUTPUT_EVENT_UNSUPPORTED");
    return 0;
}
static void notification(void *self, void *cmd, long type, void *property_dict, void *which) {
    (void)self; (void)cmd; (void)property_dict; (void)which;
    printf("VTOUCH_NOTIFICATION=%ld\n", type);
    if (type == 10 || type == 11) atomic_store(&state, (int)type);
}

uint64_t virtual_touch_start(void) {
    void *hid = dlopen("/System/Library/PrivateFrameworks/HID.framework/HID", RTLD_NOW);
    if (!hid) { printf("VTOUCH_DLOPEN_ERROR=%s\n", dlerror()); return 0; }
    message = dlsym(RTLD_DEFAULT, "objc_msgSend");
    selector = dlsym(RTLD_DEFAULT, "sel_registerName");
    class_named = dlsym(RTLD_DEFAULT, "objc_getClass");
    void *(*allocate)(void *, const char *, size_t) = dlsym(RTLD_DEFAULT, "objc_allocateClassPair");
    unsigned char (*add_method)(void *, void *, void *, const char *) = dlsym(RTLD_DEFAULT, "class_addMethod");
    void (*register_class)(void *) = dlsym(RTLD_DEFAULT, "objc_registerClassPair");
    void *(*get_protocol)(const char *) = dlsym(RTLD_DEFAULT, "objc_getProtocol");
    unsigned char (*add_protocol)(void *, void *) = dlsym(RTLD_DEFAULT, "class_addProtocol");
    void *(*queue_create)(const char *, void *) = dlsym(RTLD_DEFAULT, "dispatch_queue_create");
    if (!message || !selector || !class_named || !allocate || !add_method ||
        !register_class || !get_protocol || !add_protocol || !queue_create) {
        puts("VTOUCH_ERROR=missing-runtime-symbol"); return 0;
    }
    void *type = allocate(class_named("NSObject"), "A19VirtualTouchDelegate", 0);
    void *protocol = get_protocol("HIDVirtualEventServiceDelegate");
    if (!type) { puts("VTOUCH_ERROR=allocate-delegate-class"); return 0; }
    /* 24A437 exposes the service class but objc_getProtocol returns nil for
     * its delegate protocol. Method implementations still work; adopting
     * available metadata is optional, as in ordinary Objective-C messaging. */
    printf("VTOUCH_DELEGATE_PROTOCOL_PRESENT=%d\n", protocol != NULL);
    if (protocol && !add_protocol(type, protocol)) {
        puts("VTOUCH_ERROR=add-delegate-protocol"); return 0;
    }
    if (!add_method(type, selector("propertyForKey:forService:"), (void *)property, "@@:@@") ||
        !add_method(type, selector("setProperty:forKey:forService:"), (void *)set_property, "B@:@@@") ||
        !add_method(type, selector("copyEventMatching:forService:"), (void *)copy_event, "@@:@@") ||
        !add_method(type, selector("setOutputEvent:forService:"), (void *)output_event, "B@:@@") ||
        !add_method(type, selector("notification:withProperty:forService:"), (void *)notification, "v@:q@@")) return 0;
    register_class(type);
    delegate = get(get(type, "alloc"), "init");
    properties = get(get(class_named("NSMutableDictionary"), "alloc"), "init");
    if (!delegate || !properties) return 0;
    const char *keys[] = {"PrimaryUsagePage", "PrimaryUsage", "VendorID", "ProductID", "MaxContactCount"};
    unsigned values[] = {13, 4, 0xffff, 0xa019, 1};
    for (unsigned i = 0; i < 5; ++i) put(properties, string(keys[i]), number(values[i]));
    put(properties, string("Product"), string("darwin-vm diagnostic touchscreen"));
    put(properties, string("Transport"), string("Virtual"));
    void *yes = ((void *(*)(void *, void *, unsigned char))message)(class_named("NSNumber"), selector("numberWithBool:"), 1);
    put(properties, string("DisplayIntegrated"), yes);
    put(properties, string("Built-In"), yes);
    void *usage = get(class_named("NSMutableDictionary"), "dictionary");
    put(usage, string("DeviceUsagePage"), number(13));
    put(usage, string("DeviceUsage"), number(4));
    void *pairs = ((void *(*)(void *, void *, void *))message)(class_named("NSArray"), selector("arrayWithObject:"), usage);
    put(properties, string("DeviceUsagePairs"), pairs);
    service = get(get(class_named("HIDVirtualEventService"), "alloc"), "init");
    printf("VTOUCH_SERVICE_OBJECT=%d\n", service != NULL);
    if (!service) return 0;
    void *queue = queue_create("darwin-vm.virtual-touch", NULL);
    if (!queue) return 0;
    set(service, "setDelegate:", delegate);
    set(service, "setDispatchQueue:", queue);
    call(service, "activate");
    for (unsigned i = 0; i < 100 && atomic_load(&state) == 0; ++i) usleep(100000);
    int current = atomic_load(&state);
    uint64_t identifier = ((uint64_t (*)(void *, void *))message)(service, selector("serviceID"));
    printf("VTOUCH_STATE=%d SERVICE_ID=0x%llx\n", current, (unsigned long long)identifier);
    if (current != 10 || !identifier) { virtual_touch_stop(); return 0; }
    return identifier;
}

int virtual_touch_dispatch(void *event) {
    if (!service || atomic_load(&state) != 10) return 0;
    return ((unsigned char (*)(void *, void *, void *))message)(service, selector("dispatchEvent:"), event);
}

void virtual_touch_stop(void) {
    if (service) {
        call(service, "cancel");
        service = NULL;
        /* Keep delegate/properties alive through asynchronous cancellation;
         * the bounded probe process owns and reclaims them on exit. */
        usleep(200000);
    }
}
