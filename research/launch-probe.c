/* Minimal research launchctl substitute using the iOS legacy launch API.
 * list: enumerate the caller's bootstrap domain. submit FILE: submit a plist.
 * This does not switch domains or reproduce launchctl's policy preprocessing.
 * Link against matching restore-image CoreFoundation and libSystem.
 */
typedef __SIZE_TYPE__ size_t;
typedef const void *CFTypeRef;
typedef unsigned long CFTypeID;
typedef long CFIndex;
typedef void *launch_data_t;
extern CFTypeID CFGetTypeID(CFTypeRef), CFDictionaryGetTypeID(void), CFArrayGetTypeID(void);
extern CFTypeID CFStringGetTypeID(void), CFBooleanGetTypeID(void), CFNumberGetTypeID(void);
extern CFIndex CFDictionaryGetCount(CFTypeRef), CFArrayGetCount(CFTypeRef);
extern void CFDictionaryGetKeysAndValues(CFTypeRef, const void **, const void **);
extern CFTypeRef CFArrayGetValueAtIndex(CFTypeRef, CFIndex);
extern unsigned char CFStringGetCString(CFTypeRef, char *, CFIndex, unsigned);
extern unsigned char CFBooleanGetValue(CFTypeRef), CFNumberIsFloatType(CFTypeRef);
extern unsigned char CFNumberGetValue(CFTypeRef, int, void *);
extern CFTypeRef CFDataCreate(CFTypeRef, const unsigned char *, CFIndex);
extern CFTypeRef CFPropertyListCreateWithData(CFTypeRef, CFTypeRef, unsigned long, long *, CFTypeRef *);
extern void CFRelease(CFTypeRef);
extern launch_data_t launch_data_alloc(int), launch_data_new_string(const char *);
extern launch_data_t launch_data_new_integer(long long), launch_data_new_real(double), launch_data_new_bool(_Bool);
extern _Bool launch_data_dict_insert(launch_data_t, launch_data_t, const char *);
extern _Bool launch_data_array_set_index(launch_data_t, launch_data_t, size_t);
extern launch_data_t launch_data_dict_lookup(launch_data_t, const char *), launch_msg(launch_data_t);
extern void launch_data_dict_iterate(launch_data_t, void (*)(launch_data_t, const char *, void *), void *);
extern int launch_data_get_type(launch_data_t), launch_data_get_errno(launch_data_t);
extern long long launch_data_get_integer(launch_data_t);
extern size_t launch_data_dict_get_count(launch_data_t);
extern void launch_data_free(launch_data_t);
extern int printf(const char *, ...), strcmp(const char *, const char *);
extern void perror(const char *), *malloc(size_t), free(void *);
extern int open(const char *, int, ...), close(int);
extern long read(int, void *, size_t);

static launch_data_t convert(CFTypeRef value) {
    CFTypeID type = CFGetTypeID(value);
    if (type == CFStringGetTypeID()) {
        char str[8192];
        if (!CFStringGetCString(value, str, sizeof(str), 0x08000100)) return 0;
        return launch_data_new_string(str);
    }
    if (type == CFBooleanGetTypeID()) return launch_data_new_bool(CFBooleanGetValue(value));
    if (type == CFNumberGetTypeID()) {
        if (CFNumberIsFloatType(value)) {
            double number; if (!CFNumberGetValue(value, 13, &number)) return 0;
            return launch_data_new_real(number);
        }
        long long number; if (!CFNumberGetValue(value, 4, &number)) return 0;
        return launch_data_new_integer(number);
    }
    if (type == CFDictionaryGetTypeID()) {
        CFIndex count = CFDictionaryGetCount(value);
        const void **keys = malloc((size_t)(count ? count : 1) * sizeof(void *));
        const void **values = malloc((size_t)(count ? count : 1) * sizeof(void *));
        if (!keys || !values) { free(keys); free(values); return 0; }
        CFDictionaryGetKeysAndValues(value, keys, values);
        launch_data_t result = launch_data_alloc(1);
        for (CFIndex i = 0; result && i < count; ++i) {
            char key[1024];
            launch_data_t item = convert(values[i]);
            if (!item || !CFStringGetCString(keys[i], key, sizeof(key), 0x08000100) ||
                !launch_data_dict_insert(result, item, key)) {
                if (item) launch_data_free(item);
                launch_data_free(result); result = 0; break;
            }
        }
        free(keys); free(values); return result;
    }
    if (type == CFArrayGetTypeID()) {
        launch_data_t result = launch_data_alloc(2);
        for (CFIndex i = 0; result && i < CFArrayGetCount(value); ++i) {
            launch_data_t item = convert(CFArrayGetValueAtIndex(value, i));
            if (!item || !launch_data_array_set_index(result, item, (size_t)i)) {
                if (item) launch_data_free(item);
                launch_data_free(result); return 0;
            }
        }
        return result;
    }
    return 0;
}
static void print_job(launch_data_t job, const char *label, void *context) {
    (void)context;
    launch_data_t pid = launch_data_dict_lookup(job, "PID");
    launch_data_t status = launch_data_dict_lookup(job, "LastExitStatus");
    printf("JOB %s PID=%lld LAST_EXIT=%lld\n", label,
           pid ? launch_data_get_integer(pid) : -1,
           status ? launch_data_get_integer(status) : -1);
}
extern unsigned int bootstrap_port;
extern int bootstrap_look_up(unsigned int, const char *, unsigned int *);
int main(int argc, char **argv) {
    if (argc == 3 && !strcmp(argv[1], "lookup")) {
        unsigned int port = 0;
        int result = bootstrap_look_up(bootstrap_port, argv[2], &port);
        printf("LOOKUP %s RESULT=%d PORT=%u\n", argv[2], result, port);
        return result != 0;
    }
    launch_data_t request = 0;
    int submitting = argc == 3 && !strcmp(argv[1], "submit");
    if (submitting) {
        int file = open(argv[2], 0);
        if (file < 0) { perror("open plist"); return 1; }
        size_t capacity = 1024 * 1024;
        unsigned char *buffer = malloc(capacity);
        if (!buffer) { close(file); return 1; }
        size_t length = 0;
        while (length < capacity) {
            long amount = read(file, buffer + length, capacity - length);
            if (amount < 0) { perror("read plist"); free(buffer); close(file); return 1; }
            if (!amount) break;
            length += (size_t)amount;
        }
        close(file);
        if (!length || length == capacity) { free(buffer); return 1; }
        CFTypeRef data = CFDataCreate(0, buffer, (CFIndex)length); free(buffer);
        if (!data) return 1;
        CFTypeRef error = 0;
        CFTypeRef plist = CFPropertyListCreateWithData(0, data, 0, 0, &error);
        CFRelease(data);
        if (!plist) { if (error) CFRelease(error); printf("Invalid plist\n"); return 1; }
        launch_data_t job = CFGetTypeID(plist) == CFDictionaryGetTypeID() ? convert(plist) : 0;
        CFRelease(plist);
        if (!job) { printf("Unsupported plist value or conversion failure\n"); return 1; }
        request = launch_data_alloc(1);
        if (!request || !launch_data_dict_insert(request, job, "SubmitJob")) {
            launch_data_free(job); if (request) launch_data_free(request); return 1;
        }
    } else if (argc == 1 || (argc == 2 && !strcmp(argv[1], "list"))) {
        request = launch_data_new_string("GetJobs");
    } else { printf("usage: launch-probe [list | submit PLIST | lookup SERVICE]\n"); return 2; }
    if (!request) return 1;
    launch_data_t reply = launch_msg(request); launch_data_free(request);
    if (!reply) { perror("launch_msg"); return 1; }
    int type = launch_data_get_type(reply), result = 1;
    printf("LAUNCH_REPLY_TYPE=%d\n", type);
    if (type == 9) {
        int error = launch_data_get_errno(reply);
        printf("LAUNCH_REPLY_ERRNO=%d\n", error); result = submitting && !error ? 0 : 1;
    }
    if (type == 1 && !submitting) {
        printf("LAUNCH_JOB_COUNT=%lu\n", launch_data_dict_get_count(reply));
        launch_data_dict_iterate(reply, print_job, 0); result = 0;
    }
    launch_data_free(reply); return result;
}
