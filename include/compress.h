#ifndef COMPRESS_H
#define COMPRESS_H

#include <stdio.h>

/*
 * .huff file format
 * =================
 *
 *  Offset  Size   Field
 *  ------  ----   -----
 *  0       4      Magic: 0x48 0x55 0x46 0x46  ("HUFF")
 *  4       1      Version: 0x01
 *  5       1      padding_bits  (0-7): how many low-order bits of the last
 *                 compressed byte are padding zeros
 *  6       8      original_size (uint64_t, little-endian)
 *  14      4      freq_count: number of (symbol, freq) pairs that follow
 *  18      freq_count * (1 + 8)   symbol (uint8_t) + freq (int64_t LE)
 *  ...     ...    compressed bit stream (byte-aligned)
 *
 * The decoder rebuilds the Huffman tree from the frequency table and then
 * reads exactly original_size symbols from the bit stream.
 */

/* Compress src_fp → dst_fp.
   Returns 0 on success, -1 on error. */
int compress(FILE *src_fp, FILE *dst_fp);

/* Decompress src_fp → dst_fp.
   Returns 0 on success, -1 on error. */
int decompress(FILE *src_fp, FILE *dst_fp);

#endif /* COMPRESS_H */
