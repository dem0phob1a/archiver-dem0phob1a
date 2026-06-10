#ifndef BITSTREAM_H
#define BITSTREAM_H

#include <stdio.h>
#include <stdint.h>

/*
 * Bit-level I/O on top of FILE*.
 * Bits are packed LSB-first inside each byte.
 *
 * BitWriter – accumulates bits; flushes full bytes automatically.
 * BitReader  – refills a byte buffer from file as needed.
 */

/* ------------------------------------------------------------------ */
/*  Writer                                                              */
/* ------------------------------------------------------------------ */
typedef struct {
    FILE    *file;
    uint8_t  buf;          /* bit accumulator                  */
    int      bit_count;    /* bits currently in buf (0-7)      */
    long long bits_written; /* total bits written (stats)       */
} BitWriter;

void bitwriter_init      (BitWriter *bw, FILE *file);
int  bitwriter_write_bit (BitWriter *bw, int bit);
int  bitwriter_write_bits(BitWriter *bw, uint64_t bits, int len);
/* Flush partial byte; returns padding count (0-7) or -1 on error. */
int  bitwriter_flush     (BitWriter *bw);

/* ------------------------------------------------------------------ */
/*  Reader                                                              */
/* ------------------------------------------------------------------ */
typedef struct {
    FILE    *file;
    uint8_t  buf;
    int      bit_count;    /* bits remaining in buf (0-7)      */
    int      eof;
} BitReader;

void bitreader_init    (BitReader *br, FILE *file);
/* Stores 0 or 1 in *bit. Returns 0 on success, -1 on EOF/error. */
int  bitreader_read_bit(BitReader *br, int *bit);
int  bitreader_eof     (const BitReader *br);

#endif /* BITSTREAM_H */
