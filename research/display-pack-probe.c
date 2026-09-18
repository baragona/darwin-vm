/* Compress the fixed capture losslessly for transfer over the guest UART. */
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
    enum { SIZE = 416 * 496 * 4 };
    alarm(15);
    unlink("/private/var/tmp/display-capture.zlib");
    void *z = dlopen("/usr/lib/libz.1.dylib", RTLD_NOW);
    unsigned long (*bound)(unsigned long) = z ? dlsym(z, "compressBound") : NULL;
    int (*compress)(unsigned char *, unsigned long *, const unsigned char *, unsigned long, int) =
        z ? dlsym(z, "compress2") : NULL;
    if (!bound || !compress) return 1;
    unsigned char *raw = malloc(SIZE);
    unsigned long length = bound(SIZE);
    unsigned char *packed = malloc(length);
    if (!raw || !packed) return 1;
    FILE *input = fopen("/private/var/tmp/display-capture.bgra", "rb");
    if (!input) return 1;
    size_t read = fread(raw, 1, SIZE, input);
    int extra = fgetc(input), failed = ferror(input), closed = fclose(input);
    if (read != SIZE || extra != EOF || failed || closed) return 1;
    int status = compress(packed, &length, raw, SIZE, 6);
    if (status) { printf("PACK_ERROR=%d\n", status); return 1; }
    FILE *output = fopen("/private/var/tmp/display-capture.zlib", "wb");
    if (!output) return 1;
    size_t written = fwrite(packed, 1, length, output);
    closed = fclose(output);
    printf("PACK_RAW_BYTES=%u PACKED_BYTES=%lu WRITTEN=%zu CLOSE=%d\n", SIZE, length, written, closed);
    free(raw); free(packed);
    return written != length || closed;
}
