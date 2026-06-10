#define _POSIX_C_SOURCE 200809L
#include "../include/compress.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

static int passed = 0, total = 0;
#define TEST(name)  do { total++; printf("  %-50s", name); } while(0)
#define PASS()      do { passed++; puts("PASS"); } while(0)
#define FAIL(msg)   do { puts("FAIL: " msg); } while(0)

static FILE *make_tmp(char *path) {
    strcpy(path, "/tmp/huff_cmp_XXXXXX");
    int fd = mkstemp(path);
    if (fd < 0) return NULL;
    FILE *f = fdopen(fd, "wb");
    return f;
}

static int roundtrip(const uint8_t *data, size_t len)
{
    char sp[64], hp[64], op[64];

    FILE *sf = make_tmp(sp); if (!sf) return 0;
    if (len > 0) fwrite(data,1,len,sf);
    fclose(sf);

    FILE *hf = make_tmp(hp); if (!hf) { remove(sp); return 0; }
    fclose(hf); hf = fopen(hp,"wb");
    sf = fopen(sp,"rb");
    int rc = compress(sf, hf);
    fclose(sf); fclose(hf);
    if (rc != 0) { remove(sp); remove(hp); return 0; }

    FILE *of = make_tmp(op); if (!of) { remove(sp); remove(hp); return 0; }
    fclose(of); of = fopen(op,"wb");
    hf = fopen(hp,"rb");
    rc = decompress(hf, of);
    fclose(hf); fclose(of);
    if (rc != 0) { remove(sp); remove(hp); remove(op); return 0; }

    of = fopen(op,"rb");
    fseek(of,0,SEEK_END); long out_len = ftell(of); fseek(of,0,SEEK_SET);
    int ok = ((size_t)out_len == len);
    if (ok && len > 0) {
        uint8_t *buf = (uint8_t*)malloc(len);
        if (buf) {
            size_t got = fread(buf,1,len,of);
            ok = (got == len && memcmp(buf,data,len)==0);
            free(buf);
        } else ok = 0;
    }
    fclose(of);
    remove(sp); remove(hp); remove(op);
    return ok;
}

static void test_empty(void)    { TEST("round-trip: empty file");
    if(roundtrip(NULL,0)) PASS(); else FAIL("mismatch"); }

static void test_single(void)   { TEST("round-trip: single byte");
    uint8_t d[]={0x42}; if(roundtrip(d,1)) PASS(); else FAIL("mismatch"); }

static void test_repeated(void) { TEST("round-trip: 256 identical bytes");
    uint8_t d[256]; memset(d,'A',256); if(roundtrip(d,256)) PASS(); else FAIL("mismatch"); }

static void test_ascii(void)    { TEST("round-trip: ASCII text");
    const char *t="The quick brown fox jumps over the lazy dog. Hello, Huffman!";
    if(roundtrip((const uint8_t*)t,strlen(t))) PASS(); else FAIL("mismatch"); }

static void test_all256(void)   { TEST("round-trip: all 256 distinct bytes");
    uint8_t d[256]; for(int i=0;i<256;i++) d[i]=(uint8_t)i;
    if(roundtrip(d,256)) PASS(); else FAIL("mismatch"); }

static void test_rand1k(void)   { TEST("round-trip: 1 KiB pseudo-random binary");
    uint8_t d[1024]; uint32_t s=0xDEADBEEF;
    for(int i=0;i<1024;i++){s=s*1664525u+1013904223u;d[i]=(uint8_t)(s>>24);}
    if(roundtrip(d,1024)) PASS(); else FAIL("mismatch"); }

static void test_zeros(void)    { TEST("round-trip: 8 KiB all-zero bytes");
    uint8_t d[8192]; memset(d,0,8192); if(roundtrip(d,8192)) PASS(); else FAIL("mismatch"); }

static void test_large(void)    { TEST("round-trip: 64 KiB repeated pattern");
    size_t len=65536; uint8_t *d=(uint8_t*)malloc(len); assert(d);
    for(size_t i=0;i<len;i++) d[i]=(uint8_t)(i%26+'a');
    int ok=roundtrip(d,len); free(d);
    if(ok) PASS(); else FAIL("mismatch"); }

static void test_two_sym(void)  { TEST("round-trip: 2 distinct byte values");
    uint8_t d[512]; for(int i=0;i<512;i++) d[i]=(i&1)?0xAA:0x55;
    if(roundtrip(d,512)) PASS(); else FAIL("mismatch"); }

static void test_smaller(void)  {
    TEST("compressed size < original for skewed distribution");
    size_t len=10000; uint8_t *d=(uint8_t*)malloc(len); assert(d);
    for(size_t i=0;i<len;i++) d[i]=(i%100<90)?'a':(i%100<97)?'b':'c';
    char sp[64],hp[64];
    FILE *sf=make_tmp(sp); fwrite(d,1,len,sf); fclose(sf); free(d);
    sf=fopen(sp,"rb");
    FILE *hf=make_tmp(hp); fclose(hf); hf=fopen(hp,"wb");
    compress(sf,hf); fclose(sf); fclose(hf);
    FILE *h=fopen(hp,"rb"); fseek(h,0,SEEK_END); long hl=ftell(h); fclose(h);
    remove(sp); remove(hp);
    if((size_t)hl<len) PASS(); else FAIL("not smaller"); }

static void test_bad_magic(void) {
    TEST("decompress rejects invalid magic");
    char p[64],o[64];
    FILE *f=make_tmp(p); fwrite("BADD",1,4,f); fclose(f);
    FILE *out=make_tmp(o); fclose(out); out=fopen(o,"wb");
    f=fopen(p,"rb"); int rc=decompress(f,out);
    fclose(f); fclose(out); remove(p); remove(o);
    if(rc!=0) PASS(); else FAIL("expected error"); }

int main(void)
{
    puts("=== Compress / Decompress Integration Tests ===");
    test_empty(); test_single(); test_repeated(); test_ascii();
    test_all256(); test_rand1k(); test_zeros(); test_large();
    test_two_sym(); test_smaller(); test_bad_magic();
    printf("\n%d / %d passed.\n", passed, total);
    return passed == total ? 0 : 1;
}
