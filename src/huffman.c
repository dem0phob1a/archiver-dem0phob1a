#include "huffman.h"
#include "pqueue.h"
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
static HuffNode *node_new(int symbol, long long freq)
{
    HuffNode *n = (HuffNode *)malloc(sizeof(HuffNode));
    if (!n) return NULL;
    n->symbol = symbol;
    n->freq   = freq;
    n->left   = n->right = NULL;
    return n;
}

/*
 * DFS to assign codes.
 * `bits`  – accumulated code word, LSB = first emitted bit.
 * `depth` – current tree depth (= code length so far).
 *
 * If root is itself a leaf (single distinct symbol), depth==0 and we
 * assign the fixed code "0" of length 1 so the encoder/decoder work.
 */
static void build_codes_rec(const HuffNode *node,
                             HuffCode codes[ALPHABET_SIZE],
                             uint64_t bits, int depth)
{
    if (!node) return;

    if (!node->left && !node->right) {
        /* Leaf – only assign if it's a real symbol (not a dummy) */
        if (node->symbol >= 0) {
            codes[node->symbol].bits = bits;
            codes[node->symbol].len  = (depth == 0) ? 1 : depth;
        }
        return;
    }

    build_codes_rec(node->left,  codes, bits,                depth + 1);
    build_codes_rec(node->right, codes, bits | (1ULL<<depth), depth + 1);
}

/* ------------------------------------------------------------------ */
void huffman_count_freq(const uint8_t *data, size_t len,
                        long long freq[ALPHABET_SIZE])
{
    for (size_t i = 0; i < len; i++)
        freq[(unsigned char)data[i]]++;
}

HuffNode *huffman_build_tree(const long long freq[ALPHABET_SIZE])
{
    PQueue *pq = pqueue_create(ALPHABET_SIZE);
    if (!pq) return NULL;

    int pushed = 0;
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (freq[i] <= 0) continue;
        HuffNode *n = node_new(i, freq[i]);
        if (!n || pqueue_insert(pq, n, (int)freq[i]) != 0) {
            if (n) free(n);
            while (pqueue_size(pq))
                huffman_free_tree((HuffNode *)pqueue_pop(pq, NULL));
            pqueue_destroy(pq);
            return NULL;
        }
        pushed++;
    }

    if (pushed == 0) { pqueue_destroy(pq); return NULL; }

    /*
     * Single distinct symbol: the tree is just the leaf itself.
     * build_codes_rec will assign it code "0" (length 1).
     * The decoder reads 1 bit per symbol and always lands on this leaf.
     */
    if (pushed == 1) {
        HuffNode *root = (HuffNode *)pqueue_pop(pq, NULL);
        pqueue_destroy(pq);
        return root;
    }

    /* General case: merge pairs until one node remains. */
    while (pqueue_size(pq) > 1) {
        HuffNode *left  = (HuffNode *)pqueue_pop(pq, NULL);
        HuffNode *right = (HuffNode *)pqueue_pop(pq, NULL);
        long long cf    = left->freq + right->freq;
        int       cp    = (cf > (long long)0x7FFFFFFF) ? 0x7FFFFFFF : (int)cf;

        HuffNode *par = node_new(-1, cf);
        if (!par || pqueue_insert(pq, par, cp) != 0) {
            if (par) free(par);
            huffman_free_tree(left);
            huffman_free_tree(right);
            while (pqueue_size(pq))
                huffman_free_tree((HuffNode *)pqueue_pop(pq, NULL));
            pqueue_destroy(pq);
            return NULL;
        }
        par->left  = left;
        par->right = right;
    }

    HuffNode *root = (HuffNode *)pqueue_pop(pq, NULL);
    pqueue_destroy(pq);
    return root;
}

void huffman_free_tree(HuffNode *root)
{
    if (!root) return;
    huffman_free_tree(root->left);
    huffman_free_tree(root->right);
    free(root);
}

void huffman_build_codes(const HuffNode *root, HuffCode codes[ALPHABET_SIZE])
{
    memset(codes, 0, ALPHABET_SIZE * sizeof(HuffCode));
    if (root) build_codes_rec(root, codes, 0, 0);
}
