#include "../include/huffman.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

static int passed = 0, total = 0;
#define TEST(name)  do { total++; printf("  %-50s", name); } while(0)
#define PASS()      do { passed++; puts("PASS"); } while(0)
#define FAIL(msg)   do { puts("FAIL: " msg); } while(0)

static void test_count_freq(void)
{
    TEST("count_freq basic");
    uint8_t buf[] = { 'a','a','b','c','c','c' };
    long long freq[ALPHABET_SIZE] = {0};
    huffman_count_freq(buf, sizeof(buf), freq);
    if (freq['a']==2 && freq['b']==1 && freq['c']==3 && freq['d']==0)
        PASS();
    else FAIL("wrong counts");
}

static void test_null_on_empty(void)
{
    TEST("build_tree returns NULL for empty freq table");
    long long freq[ALPHABET_SIZE] = {0};
    HuffNode *root = huffman_build_tree(freq);
    if (!root) PASS(); else { FAIL("expected NULL"); huffman_free_tree(root); }
}

static void test_single_symbol(void)
{
    TEST("single symbol gets code of length >= 1");
    long long freq[ALPHABET_SIZE] = {0};
    freq['x'] = 10;
    HuffNode *root = huffman_build_tree(freq);
    assert(root);
    HuffCode codes[ALPHABET_SIZE];
    huffman_build_codes(root, codes);
    huffman_free_tree(root);
    if (codes['x'].len >= 1) PASS(); else FAIL("len < 1");
}

static void test_prefix_free(void)
{
    TEST("codes are prefix-free");
    const char *text = "abracadabra";
    long long freq[ALPHABET_SIZE] = {0};
    huffman_count_freq((const uint8_t*)text, strlen(text), freq);
    HuffNode *root = huffman_build_tree(freq);
    assert(root);
    HuffCode codes[ALPHABET_SIZE];
    huffman_build_codes(root, codes);
    huffman_free_tree(root);

    int ok = 1;
    for (int i = 0; i < ALPHABET_SIZE && ok; i++) {
        if (!codes[i].len) continue;
        for (int j = 0; j < ALPHABET_SIZE && ok; j++) {
            if (i == j || !codes[j].len) continue;
            int li = codes[i].len, lj = codes[j].len;
            if (li <= lj) {
                uint64_t mask   = (1ULL << li) - 1;
                uint64_t prefix = codes[j].bits & mask;
                if (prefix == codes[i].bits) ok = 0;
            }
        }
    }
    if (ok) PASS(); else FAIL("found prefix collision");
}

static void test_freq_len_relation(void)
{
    TEST("higher frequency → shorter or equal code length");
    long long freq[ALPHABET_SIZE] = {0};
    freq['a'] = 50; freq['b'] = 20; freq['c'] = 5;
    HuffNode *root = huffman_build_tree(freq);
    assert(root);
    HuffCode codes[ALPHABET_SIZE];
    huffman_build_codes(root, codes);
    huffman_free_tree(root);
    if (codes['a'].len <= codes['b'].len && codes['b'].len <= codes['c'].len)
        PASS();
    else FAIL("length-frequency relation violated");
}

static void test_all_256_symbols(void)
{
    TEST("all 256 symbols get codes");
    long long freq[ALPHABET_SIZE];
    for (int i = 0; i < ALPHABET_SIZE; i++) freq[i] = i + 1;
    HuffNode *root = huffman_build_tree(freq);
    assert(root);
    HuffCode codes[ALPHABET_SIZE];
    huffman_build_codes(root, codes);
    huffman_free_tree(root);
    int ok = 1;
    for (int i = 0; i < ALPHABET_SIZE; i++)
        if (!codes[i].len) { ok = 0; break; }
    if (ok) PASS(); else FAIL("missing code for some symbol");
}

int main(void)
{
    puts("=== Huffman Tree Tests ===");
    test_count_freq();
    test_null_on_empty();
    test_single_symbol();
    test_prefix_free();
    test_freq_len_relation();
    test_all_256_symbols();
    printf("\n%d / %d passed.\n", passed, total);
    return passed == total ? 0 : 1;
}
