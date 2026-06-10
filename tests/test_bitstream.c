#define _POSIX_C_SOURCE 200809L
#include "../include/bitstream.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

static int passed = 0, total = 0;
#define TEST(name)  do { total++; printf("  %-50s", name); } while(0)
#define PASS()      do { passed++; puts("PASS"); } while(0)
#define FAIL(msg)   do { puts("FAIL: " msg); } while(0)

static FILE *make_tmp(char *path) {
    strcpy(path, "/tmp/huff_bs_XXXXXX");
    int fd = mkstemp(path);
    if (fd < 0) return NULL;
    return fdopen(fd, "wb");
}

static void test_write_read_one_byte(void)
{
    TEST("write/read 8 bits = 1 byte");
    char path[64]; FILE *wf = make_tmp(path); assert(wf);
    BitWriter bw; bitwriter_init(&bw, wf);
    uint8_t pattern = 0xB4;
    for (int i = 0; i < 8; i++) bitwriter_write_bit(&bw, (pattern >> i) & 1);
    bitwriter_flush(&bw); fclose(wf);

    FILE *rf = fopen(path,"rb"); assert(rf);
    BitReader br; bitreader_init(&br, rf);
    uint8_t got = 0; int ok = 1;
    for (int i = 0; i < 8; i++) {
        int bit; if (bitreader_read_bit(&br,&bit)!=0){ok=0;break;}
        got |= (uint8_t)(bit << i);
    }
    fclose(rf); remove(path);
    if (ok && got == pattern) PASS(); else FAIL("byte mismatch");
}

static void test_non_aligned(void)
{
    TEST("write/read 13 non-aligned bits");
    char path[64]; FILE *wf = make_tmp(path); assert(wf);
    BitWriter bw; bitwriter_init(&bw, wf);
    uint64_t bits = 0x1A3B & 0x1FFF;
    bitwriter_write_bits(&bw, bits, 13);
    int pad = bitwriter_flush(&bw); fclose(wf);

    FILE *rf = fopen(path,"rb"); assert(rf);
    BitReader br; bitreader_init(&br, rf);
    uint64_t got = 0;
    for (int i = 0; i < 13; i++) {
        int bit; bitreader_read_bit(&br,&bit); got |= ((uint64_t)bit << i);
    }
    fclose(rf); remove(path);
    if ((got & 0x1FFF) == bits && pad == 3) PASS(); else FAIL("data or padding");
}

static void test_padding_count(void)
{
    TEST("flush returns correct padding (5 bits -> 3 padding)");
    char path[64]; FILE *wf = make_tmp(path); assert(wf);
    BitWriter bw; bitwriter_init(&bw, wf);
    bitwriter_write_bits(&bw, 0x15, 5);
    int pad = bitwriter_flush(&bw); fclose(wf); remove(path);
    if (pad == 3) PASS(); else FAIL("expected 3");
}

static void test_no_padding_aligned(void)
{
    TEST("flush returns 0 padding when byte-aligned");
    char path[64]; FILE *wf = make_tmp(path); assert(wf);
    BitWriter bw; bitwriter_init(&bw, wf);
    bitwriter_write_bits(&bw, 0xFF, 8);
    int pad = bitwriter_flush(&bw); fclose(wf); remove(path);
    if (pad == 0) PASS(); else FAIL("expected 0");
}

static void test_eof_detection(void)
{
    TEST("bitreader detects EOF after last bit");
    char path[64]; FILE *wf = make_tmp(path); assert(wf);
    BitWriter bw; bitwriter_init(&bw, wf);
    bitwriter_write_bits(&bw, 0xAB, 8);
    bitwriter_flush(&bw); fclose(wf);

    FILE *rf = fopen(path,"rb"); assert(rf);
    BitReader br; bitreader_init(&br, rf);
    int bit; for (int i = 0; i < 8; i++) bitreader_read_bit(&br,&bit);
    int rc = bitreader_read_bit(&br,&bit);
    int eof = bitreader_eof(&br);
    fclose(rf); remove(path);
    if (rc == -1 && eof) PASS(); else FAIL("EOF not detected");
}

static void test_alternating_1000(void)
{
    TEST("write/read 1000 alternating bits");
    char path[64]; FILE *wf = make_tmp(path); assert(wf);
    BitWriter bw; bitwriter_init(&bw, wf);
    for (int i = 0; i < 1000; i++) bitwriter_write_bit(&bw, i & 1);
    bitwriter_flush(&bw); fclose(wf);

    FILE *rf = fopen(path,"rb"); assert(rf);
    BitReader br; bitreader_init(&br, rf);
    int ok = 1;
    for (int i = 0; i < 1000; i++) {
        int bit; if (bitreader_read_bit(&br,&bit)!=0||bit!=(i&1)){ok=0;break;}
    }
    fclose(rf); remove(path);
    if (ok) PASS(); else FAIL("mismatch");
}

static void test_all_zeros(void)
{
    TEST("write/read 64 zero bits");
    char path[64]; FILE *wf = make_tmp(path); assert(wf);
    BitWriter bw; bitwriter_init(&bw, wf);
    bitwriter_write_bits(&bw, 0ULL, 64);
    bitwriter_flush(&bw); fclose(wf);
    FILE *rf = fopen(path,"rb"); assert(rf);
    BitReader br; bitreader_init(&br, rf); int ok = 1;
    for (int i = 0; i < 64; i++) {
        int bit; if (bitreader_read_bit(&br,&bit)!=0||bit!=0){ok=0;break;}
    }
    fclose(rf); remove(path);
    if (ok) PASS(); else FAIL("non-zero bit");
}

static void test_all_ones(void)
{
    TEST("write/read 64 one bits");
    char path[64]; FILE *wf = make_tmp(path); assert(wf);
    BitWriter bw; bitwriter_init(&bw, wf);
    bitwriter_write_bits(&bw, ~0ULL, 64);
    bitwriter_flush(&bw); fclose(wf);
    FILE *rf = fopen(path,"rb"); assert(rf);
    BitReader br; bitreader_init(&br, rf); int ok = 1;
    for (int i = 0; i < 64; i++) {
        int bit; if (bitreader_read_bit(&br,&bit)!=0||bit!=1){ok=0;break;}
    }
    fclose(rf); remove(path);
    if (ok) PASS(); else FAIL("zero bit read back");
}

int main(void)
{
    puts("=== Bitstream Tests ===");
    test_write_read_one_byte();
    test_non_aligned();
    test_padding_count();
    test_no_padding_aligned();
    test_eof_detection();
    test_alternating_1000();
    test_all_zeros();
    test_all_ones();
    printf("\n%d / %d passed.\n", passed, total);
    return passed == total ? 0 : 1;
}
