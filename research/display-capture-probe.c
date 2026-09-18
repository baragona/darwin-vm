/* Experimental 24A437 display capture; API success does not imply visible UI.
 * CARenderServerRenderDisplay wrapper at 0x1846dbefc takes server port,
 * display-name CFString, IOSurface, x offset, y offset and returns bool.
 * Captures the diagnostic 416x496 LCD, not QEMU's boot framebuffer.
 */
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(void) {
    enum { WIDTH = 416, HEIGHT = 496, ROW_BYTES = WIDTH * 4 };
    setbuf(stdout, NULL);
    alarm(30);
    puts("DISPLAY_CAPTURE_BEGIN");
    unlink("/private/var/tmp/display-capture.bgra");
    void *qc = dlopen("/System/Library/Frameworks/QuartzCore.framework/QuartzCore", RTLD_NOW);
    void *io = dlopen("/System/Library/Frameworks/IOSurface.framework/IOSurface", RTLD_NOW);
    if (!qc || !io) { printf("DLOPEN_ERROR=%s\n", dlerror()); return 1; }
    void *(*cls)(const char *) = (void *(*)(const char *))dlsym(RTLD_DEFAULT, "objc_getClass");
    void *(*sel)(const char *) = (void *(*)(const char *))dlsym(RTLD_DEFAULT, "sel_registerName");
    void *msg = dlsym(RTLD_DEFAULT, "objc_msgSend");
    void *(*create)(void *) = (void *(*)(void *))dlsym(io, "IOSurfaceCreate");
    int (*lock)(void *, uint32_t, uint32_t *) = (int (*)(void *, uint32_t, uint32_t *))dlsym(io, "IOSurfaceLock");
    int (*unlock)(void *, uint32_t, uint32_t *) = (int (*)(void *, uint32_t, uint32_t *))dlsym(io, "IOSurfaceUnlock");
    void *(*base)(void *) = (void *(*)(void *))dlsym(io, "IOSurfaceGetBaseAddress");
    size_t (*size)(void *) = (size_t (*)(void *))dlsym(io, "IOSurfaceGetAllocSize");
    size_t (*stride)(void *) = (size_t (*)(void *))dlsym(io, "IOSurfaceGetBytesPerRow");
    _Bool (*render)(uint32_t, void *, void *, int, int) =
        (_Bool (*)(uint32_t, void *, void *, int, int))dlsym(qc, "CARenderServerRenderDisplay");
    if (!cls || !sel || !msg || !create || !lock || !unlock || !base || !size || !stride || !render) {
        puts("DISPLAY_CAPTURE_SYMBOLS_MISSING"); return 1;
    }
    void *(*get)(void *, void *) = (void *(*)(void *, void *))msg;
    void *display = get(cls("CADisplay"), sel("mainDisplay"));
    if (!display) { puts("DISPLAY_CAPTURE_NO_MAIN_DISPLAY"); return 1; }
    void *name = get(display, sel("name"));
    if (!name) return 1;
    const char *text = ((const char *(*)(void *, void *))msg)(name, sel("UTF8String"));
    printf("DISPLAY_CAPTURE_NAME=%s WIDTH=%u HEIGHT=%u\n", text ? text : "(nil)", WIDTH, HEIGHT);
    if (!text || strcmp(text, "LCD")) return 1;
    void *dict = get(cls("NSMutableDictionary"), sel("dictionary"));
    const char *keys[] = {"kIOSurfaceWidth", "kIOSurfaceHeight", "kIOSurfaceBytesPerElement",
        "kIOSurfaceBytesPerRow", "kIOSurfaceAllocSize", "kIOSurfacePixelFormat"};
    unsigned values[] = {WIDTH, HEIGHT, 4, ROW_BYTES, ROW_BYTES * HEIGHT, 0x42475241};
    for (unsigned i = 0; i < 6; i++) {
        void **key = dlsym(io, keys[i]);
        if (!key) return 1;
        void *number = ((void *(*)(void *, void *, unsigned))msg)(cls("NSNumber"), sel("numberWithUnsignedInt:"), values[i]);
        ((void (*)(void *, void *, void *, void *))msg)(dict, sel("setObject:forKey:"), number, *key);
    }
    void *surface = create(dict);
    printf("DISPLAY_CAPTURE_SURFACE=%d\n", surface != NULL);
    if (!surface) return 1;
    size_t row = stride(surface), length = size(surface);
    if (row < ROW_BYTES || row > 16384 || length < row * HEIGHT || length > 16 * 1024 * 1024) return 1;
    if (lock(surface, 0, NULL)) return 1;
    unsigned char *pixels = base(surface);
    if (!pixels) return 1;
    memset(pixels, 0xa5, length);
    if (unlock(surface, 0, NULL)) return 1;
    puts("DISPLAY_CAPTURE_RENDER_BEGIN");
    int rendered = render(0, name, surface, 0, 0);
    printf("DISPLAY_CAPTURE_RENDER_RESULT=%d\n", rendered);
    if (!rendered || lock(surface, 0, NULL)) return 1;
    pixels = base(surface);
    if (!pixels) return 1;
    size_t changed = 0, colored = 0, opaque = 0;
    for (unsigned y = 0; y < HEIGHT; y++) for (unsigned x = 0; x < WIDTH; x++) {
        unsigned char *p = pixels + y * row + x * 4;
        int differs = p[0] != 0xa5 || p[1] != 0xa5 || p[2] != 0xa5 || p[3] != 0xa5;
        changed += differs;
        colored += differs && (p[0] || p[1] || p[2]);
        opaque += p[3] == 255;
    }
    printf("DISPLAY_CAPTURE_CHANGED_PIXELS=%zu COLORED_PIXELS=%zu OPAQUE_PIXELS=%zu\n", changed, colored, opaque);
    FILE *file = fopen("/private/var/tmp/display-capture.bgra", "wb");
    if (!file) { perror("capture output"); return 1; }
    size_t written = 0;
    for (unsigned y = 0; y < HEIGHT; y++) written += fwrite(pixels + y * row, 1, ROW_BYTES, file);
    int closed = fclose(file), unlocked = unlock(surface, 0, NULL);
    printf("DISPLAY_CAPTURE_OUTPUT_BYTES=%zu CLOSE=%d UNLOCK=%d\n", written, closed, unlocked);
    puts("DISPLAY_CAPTURE_END");
    return written != ROW_BYTES * HEIGHT || closed || unlocked;
}
