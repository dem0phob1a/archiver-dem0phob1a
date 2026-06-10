/*
 * archiver.c — CLI front-end
 *
 * Usage:
 *   archiver compress   <input>  <output.huff>
 *   archiver decompress <input.huff> <output>
 *   archiver info       <input.huff>
 */

#include "compress.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(const char *argv0)
{
    fprintf(stderr,
        "Usage:\n"
        "  %s compress   <input>       <output.huff>\n"
        "  %s decompress <input.huff>  <output>\n"
        "  %s info       <input.huff>\n",
        argv0, argv0, argv0);
}

/* Return file size in bytes, or -1 on error. */
static long file_size(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fclose(f);
    return sz;
}

int main(int argc, char *argv[])
{
    if (argc < 2) { usage(argv[0]); return 1; }

    const char *cmd = argv[1];

    /* ------------------------------------------------------------------ */
    if (strcmp(cmd, "compress") == 0) {
        if (argc != 4) { usage(argv[0]); return 1; }
        const char *src_path = argv[2];
        const char *dst_path = argv[3];

        FILE *src = fopen(src_path, "rb");
        if (!src) { perror(src_path); return 1; }

        FILE *dst = fopen(dst_path, "wb");
        if (!dst) { perror(dst_path); fclose(src); return 1; }

        int rc = compress(src, dst);
        fclose(src);
        fclose(dst);

        if (rc != 0) {
            fprintf(stderr, "Compression failed.\n");
            return 1;
        }

        long orig = file_size(src_path);
        long comp = file_size(dst_path);
        if (orig > 0 && comp > 0) {
            printf("Compressed: %s → %s\n", src_path, dst_path);
            printf("Original : %ld bytes\n", orig);
            printf("Compressed: %ld bytes\n", comp);
            printf("Ratio    : %.2f%%\n", 100.0 * comp / orig);
        }
        return 0;
    }

    /* ------------------------------------------------------------------ */
    if (strcmp(cmd, "decompress") == 0) {
        if (argc != 4) { usage(argv[0]); return 1; }
        const char *src_path = argv[2];
        const char *dst_path = argv[3];

        FILE *src = fopen(src_path, "rb");
        if (!src) { perror(src_path); return 1; }

        FILE *dst = fopen(dst_path, "wb");
        if (!dst) { perror(dst_path); fclose(src); return 1; }

        int rc = decompress(src, dst);
        fclose(src);
        fclose(dst);

        if (rc != 0) {
            fprintf(stderr, "Decompression failed.\n");
            return 1;
        }

        printf("Decompressed: %s → %s\n", src_path, dst_path);
        return 0;
    }

    /* ------------------------------------------------------------------ */
    if (strcmp(cmd, "info") == 0) {
        if (argc != 3) { usage(argv[0]); return 1; }
        const char *path = argv[2];

        FILE *f = fopen(path, "rb");
        if (!f) { perror(path); return 1; }

        /* Read magic */
        uint8_t magic[4];
        if (fread(magic, 1, 4, f) != 4 ||
            magic[0] != 0x48 || magic[1] != 0x55 ||
            magic[2] != 0x46 || magic[3] != 0x46) {
            fprintf(stderr, "Not a .huff file.\n");
            fclose(f);
            return 1;
        }

        uint8_t version  = (uint8_t)fgetc(f);
        uint8_t padding  = (uint8_t)fgetc(f);

        uint8_t sb[8];
        fread(sb, 1, 8, f);
        uint64_t orig_size = 0;
        for (int i = 0; i < 8; i++) orig_size |= ((uint64_t)sb[i]) << (8*i);

        uint8_t cb[4];
        fread(cb, 1, 4, f);
        uint32_t freq_count = (uint32_t)cb[0] | ((uint32_t)cb[1]<<8)
                            | ((uint32_t)cb[2]<<16) | ((uint32_t)cb[3]<<24);

        long comp_size = file_size(path);
        fclose(f);

        printf("File        : %s\n",   path);
        printf("Version     : %u\n",   version);
        printf("Padding bits: %u\n",   padding);
        printf("Orig. size  : %llu bytes\n", (unsigned long long)orig_size);
        printf("Comp. size  : %ld bytes\n",  comp_size);
        printf("Symbols     : %u distinct\n", freq_count);
        if (orig_size > 0)
            printf("Ratio       : %.2f%%\n",
                   100.0 * comp_size / (double)orig_size);
        return 0;
    }

    /* ------------------------------------------------------------------ */
    fprintf(stderr, "Unknown command: %s\n", cmd);
    usage(argv[0]);
    return 1;
}
