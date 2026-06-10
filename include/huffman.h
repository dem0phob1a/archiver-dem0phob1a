#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <stdint.h>
#include <stddef.h>

#define ALPHABET_SIZE 256

/* One entry in the code table.
 * bits: the code word, LSB = first bit to emit.
 * len:  number of valid bits (0 means symbol not present in input). */
typedef struct {
    uint64_t bits;
    int      len;
} HuffCode;

/* A node in the Huffman tree. */
typedef struct HuffNode {
    int              symbol;  /* byte value 0-255; -1 for internal nodes */
    long long        freq;
    struct HuffNode *left;
    struct HuffNode *right;
} HuffNode;

/* Count byte frequencies in a buffer.
 * freq[] must be zeroed by the caller; successive calls accumulate. */
void      huffman_count_freq(const uint8_t *data, size_t len,
                             long long freq[ALPHABET_SIZE]);

/* Build a Huffman tree from a frequency table.
 * Returns root, or NULL on empty input / allocation failure.
 * Caller must free with huffman_free_tree(). */
HuffNode *huffman_build_tree(const long long freq[ALPHABET_SIZE]);

/* Recursively free a Huffman tree. */
void      huffman_free_tree(HuffNode *root);

/* Derive a code table from a Huffman tree.
 * codes[] is zeroed first; symbols absent from the tree keep len == 0. */
void      huffman_build_codes(const HuffNode *root,
                              HuffCode codes[ALPHABET_SIZE]);

#endif /* HUFFMAN_H */
