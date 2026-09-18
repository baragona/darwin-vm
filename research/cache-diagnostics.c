/* Minimal iOS probe; declarations avoid requiring an installed iPhone SDK.
 * Link against libSystem from the matching restore image, then ad-hoc sign
 * and include its CDHash in the experimental guest trust cache.
 */
typedef __SIZE_TYPE__ size_t;
extern int sysctlbyname(const char *, void *, size_t *, void *, size_t);
extern int printf(const char *, ...);
extern void perror(const char *);
extern void *malloc(size_t);
extern long write(int, const void *, size_t);
int main(int argc, char **argv) {
    (void)argv;
    if (argc > 1) {
        size_t length = 0;
        if (sysctlbyname("kern.msgbuf", 0, &length, 0, 0)) {
            perror("kern.msgbuf size"); return 1;
        }
        void *buffer = malloc(length);
        if (!buffer) return 1;
        if (sysctlbyname("kern.msgbuf", buffer, &length, 0, 0)) {
            perror("kern.msgbuf read"); return 1;
        }
        write(1, buffer, length);
        return 0;
    }
    int before = -1, after = 4;
    size_t size = sizeof(before);
    if (sysctlbyname("vm.shared_region_trace_level", &before, &size,
                     &after, sizeof(after)) != 0) {
        perror("shared_region_trace_level");
        return 1;
    }
    printf("shared_region_trace_level: %d -> %d\n", before, after);
    return 0;
}
