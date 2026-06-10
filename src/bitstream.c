#include "bitstream.h"

/* ================================================================== */
/*  BitWriter                                                           */
/* ================================================================== */

void bitwriter_init(BitWriter *bw, FILE *file)
{
    bw->file        = file;
    bw->buf         = 0;
    bw->bit_count   = 0;
    bw->bits_written = 0;
}

int bitwriter_write_bit(BitWriter *bw, int bit)
{
    if (bit) bw->buf |= (uint8_t)(1u << bw->bit_count);
    bw->bit_count++;
    bw->bits_written++;
    if (bw->bit_count == 8) {
        if (fwrite(&bw->buf, 1, 1, bw->file) != 1) return -1;
        bw->buf       = 0;
        bw->bit_count = 0;
    }
    return 0;
}

int bitwriter_write_bits(BitWriter *bw, uint64_t bits, int len)
{
    for (int i = 0; i < len; i++)
        if (bitwriter_write_bit(bw, (int)((bits >> i) & 1u)) != 0) return -1;
    return 0;
}

int bitwriter_flush(BitWriter *bw)
{
    int padding = 0;
    if (bw->bit_count > 0) {
        padding = 8 - bw->bit_count;
        if (fwrite(&bw->buf, 1, 1, bw->file) != 1) return -1;
        bw->buf       = 0;
        bw->bit_count = 0;
    }
    return padding;
}

/* ================================================================== */
/*  BitReader                                                           */
/* ================================================================== */

void bitreader_init(BitReader *br, FILE *file)
{
    br->file      = file;
    br->buf       = 0;
    br->bit_count = 0;
    br->eof       = 0;
}

int bitreader_read_bit(BitReader *br, int *bit)
{
    if (br->eof) return -1;
    if (br->bit_count == 0) {
        int c = fgetc(br->file);
        if (c == EOF) { br->eof = 1; return -1; }
        br->buf       = (uint8_t)c;
        br->bit_count = 8;
    }
    *bit = br->buf & 1;
    br->buf >>= 1;
    br->bit_count--;
    return 0;
}

int bitreader_eof(const BitReader *br) { return br->eof; }
