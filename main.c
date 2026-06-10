/*
 * main.c — Huffman archiver CLI entry point.
 *
 * Commands:
 *   archiver compress   <input>      <output.huff>
 *   archiver decompress <input.huff> <output>
 *   archiver info       <input.huff>
 */

#include "compress.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static void usage(const char *prog)
{
    fprintf(stderr,
        "Huffman archiver\n"
        "Usage:\n"
        "  %s compress   <input>       <output.huff>\n"
        "  %s decompress <input.huff>  <output>\n"
        "  %s info       <input.huff>\n",
        prog, prog, prog);
}

static long get_file_size(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fclose(f);
    return sz;
}

/* ------------------------------------------------------------------ */
/*  "info" command: parse and display .huff header                     */
/* ------------------------------------------------------------------ */
static int cmd_info(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); return 1; }

    /* Magic */
    uint8_t magic[4];
    if (fread(magic, 1, 4, f) != 4 ||
        magic[0] != 'H' || magic[1] != 'U' ||
        magic[2] != 'F' || magic[3] != 'F') {
        fprintf(stderr, "info: not a .huff file.\n");
        fclose(f); return 1;
    }

    /* Version */
    int version_c = fgetc(f);
    if (version_c == EOF) { fprintf(stderr, "info: truncated header.\n"); fclose(f); return 1; }
    uint8_t version = (uint8_t)version_c;

    /* Original size (uint64 LE) */
    uint8_t sb[8];
    if (fread(sb, 1, 8, f) != 8) { fprintf(stderr, "info: truncated header.\n"); fclose(f); return 1; }
    uint64_t orig_size = 0;
    for (int i = 0; i < 8; i++) orig_size |= ((uint64_t)sb[i]) << (8*i);

    /* Distinct symbol count (uint16 LE) */
    uint8_t db[2];
    if (fread(db, 1, 2, f) != 2) { fprintf(stderr, "info: truncated header.\n"); fclose(f); return 1; }
    uint16_t distinct = (uint16_t)(db[0] | ((uint16_t)db[1] << 8));

    /* Skip symbol table: each entry is 1 + 8 bytes */
    if (fseek(f, (long)distinct * 9, SEEK_CUR) != 0) {
        fprintf(stderr, "info: seek failed.\n"); fclose(f); return 1;
    }

    /* Padding bits */
    int pad_c = fgetc(f);
    if (pad_c == EOF) { fprintf(stderr, "info: truncated header.\n"); fclose(f); return 1; }
    uint8_t padding = (uint8_t)pad_c;

    fclose(f);

    long comp_size = get_file_size(path);
    printf("File      : %s\n",  path);
    printf("Version   : %u\n",  version);
    printf("Orig size : %llu bytes\n", (unsigned long long)orig_size);
    printf("Comp size : %ld bytes\n",  comp_size);
    printf("Symbols   : %u distinct\n", (unsigned)distinct);
    printf("Padding   : %u bit(s)\n",   (unsigned)padding);
    if (orig_size > 0)
        printf("Ratio     : %.2f%% of original\n",
               100.0 * (double)comp_size / (double)orig_size);
    return 0;
}

/* ------------------------------------------------------------------ */
int main(int argc, char *argv[])
{
    if (argc < 2) { usage(argv[0]); return EXIT_FAILURE; }

    const char *cmd = argv[1];

    /* ---------- compress ---------- */
    if (strcmp(cmd, "compress") == 0) {
        if (argc != 4) { usage(argv[0]); return EXIT_FAILURE; }

        FILE *src = fopen(argv[2], "rb");
        if (!src) { perror(argv[2]); return EXIT_FAILURE; }
        FILE *dst = fopen(argv[3], "wb");
        if (!dst) { perror(argv[3]); fclose(src); return EXIT_FAILURE; }

        int rc = compress(src, dst);
        fclose(src);
        fclose(dst);

        if (rc != 0) {
            fprintf(stderr, "Compression failed.\n");
            remove(argv[3]);
            return EXIT_FAILURE;
        }

        long orig = get_file_size(argv[2]);
        long comp = get_file_size(argv[3]);
        printf("Compressed  : %s → %s\n", argv[2], argv[3]);
        if (orig > 0)
            printf("Size        : %ld → %ld bytes (%.1f%% of original)\n",
                   orig, comp, 100.0 * comp / (double)orig);
        return EXIT_SUCCESS;
    }

    /* ---------- decompress ---------- */
    if (strcmp(cmd, "decompress") == 0) {
        if (argc != 4) { usage(argv[0]); return EXIT_FAILURE; }

        FILE *src = fopen(argv[2], "rb");
        if (!src) { perror(argv[2]); return EXIT_FAILURE; }
        FILE *dst = fopen(argv[3], "wb");
        if (!dst) { perror(argv[3]); fclose(src); return EXIT_FAILURE; }

        int rc = decompress(src, dst);
        fclose(src);
        fclose(dst);

        if (rc != 0) {
            fprintf(stderr, "Decompression failed.\n");
            remove(argv[3]);
            return EXIT_FAILURE;
        }

        printf("Decompressed: %s → %s\n", argv[2], argv[3]);
        return EXIT_SUCCESS;
    }

    /* ---------- info ---------- */
    if (strcmp(cmd, "info") == 0) {
        if (argc != 3) { usage(argv[0]); return EXIT_FAILURE; }
        return cmd_info(argv[2]);
    }

    fprintf(stderr, "Unknown command: %s\n\n", cmd);
    usage(argv[0]);
    return EXIT_FAILURE;
}
