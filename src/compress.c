#include "compress.h"
#include "huffman.h"
#include "bitstream.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ------------------------------------------------------------------ */
/*  .huff file format
 *
 *  [4]  magic       "HUFF"
 *  [1]  version     0x01
 *  [8]  orig_size   uint64 LE  – original byte count
 *  [2]  distinct    uint16 LE  – number of (symbol, freq) pairs
 *  for each pair:
 *    [1]  symbol    uint8
 *    [8]  freq      int64  LE
 *  [1]  padding     uint8  – zero-pad bits appended to last byte (0-7)
 *  [N]  bitstream
 * ------------------------------------------------------------------ */

static const uint8_t MAGIC[4] = { 'H','U','F','F' };
static const uint8_t VERSION  = 0x01;
#define IO_BUF_SIZE (1 << 16)   /* 64 KiB */

/* ---- portable LE helpers ------------------------------------------ */
static int w8(FILE *f, uint8_t v)
    { return fwrite(&v,1,1,f)==1 ? 0 : -1; }
static int w16(FILE *f, uint16_t v) {
    uint8_t b[2]={(uint8_t)v,(uint8_t)(v>>8)};
    return fwrite(b,1,2,f)==2 ? 0 : -1;
}
static int w64(FILE *f, uint64_t v) {
    uint8_t b[8]; int i;
    for(i=0;i<8;i++){b[i]=(uint8_t)(v&0xFF);v>>=8;}
    return fwrite(b,1,8,f)==8 ? 0 : -1;
}
static int r8(FILE *f, uint8_t *o)
    { return fread(o,1,1,f)==1 ? 0 : -1; }
static int r16(FILE *f, uint16_t *o) {
    uint8_t b[2]; if(fread(b,1,2,f)!=2) return -1;
    *o=(uint16_t)(b[0]|((uint16_t)b[1]<<8)); return 0;
}
static int r64(FILE *f, uint64_t *o) {
    uint8_t b[8]; int i; if(fread(b,1,8,f)!=8) return -1;
    uint64_t u=0; for(i=7;i>=0;i--) u=(u<<8)|b[i]; *o=u; return 0;
}
static int wi64(FILE *f, int64_t v)  { return w64(f,(uint64_t)v); }
static int ri64(FILE *f, int64_t *o) {
    uint64_t u; if(r64(f,&u)!=0) return -1; *o=(int64_t)u; return 0;
}

/* ================================================================== */
int compress(FILE *src, FILE *dst)
{
    long long freq[ALPHABET_SIZE];
    memset(freq, 0, sizeof(freq));

    uint8_t *buf = (uint8_t *)malloc(IO_BUF_SIZE);
    if (!buf) { fputs("compress: out of memory\n",stderr); return -1; }

    /* ---- Pass 1: count frequencies ---- */
    uint64_t orig_size = 0;
    size_t n;
    while ((n = fread(buf, 1, IO_BUF_SIZE, src)) > 0) {
        huffman_count_freq(buf, n, freq);
        orig_size += n;
    }
    if (ferror(src)) { fputs("compress: read error\n",stderr); free(buf); return -1; }

    /* Empty file */
    if (orig_size == 0) {
        free(buf);
        if (fwrite(MAGIC,1,4,dst)!=4) return -1;
        if (w8(dst,VERSION)!=0) return -1;
        if (w64(dst,0)!=0)      return -1;
        if (w16(dst,0)!=0)      return -1;
        if (w8(dst,0)!=0)       return -1;
        return 0;
    }

    /* ---- Build Huffman tree and code table ---- */
    HuffNode *root = huffman_build_tree(freq);
    if (!root) { fputs("compress: build_tree failed\n",stderr); free(buf); return -1; }
    HuffCode codes[ALPHABET_SIZE];
    huffman_build_codes(root, codes);
    huffman_free_tree(root);

    /* Count distinct symbols for header */
    uint16_t distinct = 0;
    for (int i = 0; i < ALPHABET_SIZE; i++) if (freq[i] > 0) distinct++;

    /* ---- Write header ---- */
    if (fwrite(MAGIC,1,4,dst)!=4)    goto werr;
    if (w8(dst,VERSION)!=0)          goto werr;
    if (w64(dst,orig_size)!=0)       goto werr;
    if (w16(dst,distinct)!=0)        goto werr;
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (freq[i] <= 0) continue;
        if (w8(dst,(uint8_t)i)!=0)         goto werr;
        if (wi64(dst,(int64_t)freq[i])!=0) goto werr;
    }
    /* Reserve padding byte — will seek back to patch it. */
    long pad_offset = ftell(dst);
    if (w8(dst,0)!=0) goto werr;

    /* ---- Pass 2: encode ---- */
    if (fseek(src, 0, SEEK_SET) != 0) {
        fputs("compress: source not seekable\n",stderr); free(buf); return -1;
    }
    {
        BitWriter bw;
        bitwriter_init(&bw, dst);
        while ((n = fread(buf, 1, IO_BUF_SIZE, src)) > 0) {
            for (size_t i = 0; i < n; i++) {
                unsigned char c = buf[i];
                if (bitwriter_write_bits(&bw, codes[c].bits, codes[c].len) != 0)
                    goto werr;
            }
        }
        if (ferror(src)) { fputs("compress: read error\n",stderr); free(buf); return -1; }

        int padding = bitwriter_flush(&bw);

        /* Patch padding byte in header */
        if (fseek(dst, pad_offset, SEEK_SET)!=0 ||
            w8(dst,(uint8_t)padding)!=0) {
            fputs("compress: header patch failed\n",stderr); free(buf); return -1;
        }
    }
    free(buf);
    return 0;

werr:
    fputs("compress: write error\n",stderr);
    free(buf); return -1;
}

/* ================================================================== */
int decompress(FILE *src, FILE *dst)
{
    /* ---- Verify magic + version ---- */
    uint8_t magic[4];
    if (fread(magic,1,4,src)!=4 || memcmp(magic,MAGIC,4)!=0) {
        fputs("decompress: not a .huff file\n",stderr); return -1;
    }
    uint8_t version;
    if (r8(src,&version)!=0 || version!=VERSION) {
        fprintf(stderr,"decompress: unsupported version %u\n",version); return -1;
    }

    /* ---- Read header ---- */
    uint64_t orig_size;
    if (r64(src,&orig_size)!=0) { fputs("decompress: truncated header\n",stderr); return -1; }

    uint16_t distinct;
    if (r16(src,&distinct)!=0) { fputs("decompress: truncated header\n",stderr); return -1; }

    long long freq[ALPHABET_SIZE];
    memset(freq, 0, sizeof(freq));
    for (uint16_t i = 0; i < distinct; i++) {
        uint8_t sym; int64_t f;
        if (r8(src,&sym)!=0 || ri64(src,&f)!=0) {
            fputs("decompress: bad frequency table\n",stderr); return -1;
        }
        freq[(unsigned char)sym] = (long long)f;
    }

    uint8_t padding_bits;
    if (r8(src,&padding_bits)!=0) { fputs("decompress: no padding byte\n",stderr); return -1; }

    if (orig_size == 0) return 0;

    /* ---- Rebuild Huffman tree ---- */
    HuffNode *root = huffman_build_tree(freq);
    if (!root) { fputs("decompress: build_tree failed\n",stderr); return -1; }

    /* Check for single-leaf tree (one distinct symbol) */
    int single_leaf = (!root->left && !root->right);

    uint8_t *out_buf = (uint8_t *)malloc(IO_BUF_SIZE);
    if (!out_buf) {
        huffman_free_tree(root);
        fputs("decompress: out of memory\n",stderr); return -1;
    }

    BitReader br;
    bitreader_init(&br, src);

    uint64_t decoded = 0;
    size_t   buf_pos = 0;

    if (single_leaf) {
        /*
         * Special case: every encoded symbol is a single '0' bit.
         * The root is itself the leaf; just emit root->symbol orig_size times.
         */
        uint8_t sym = (uint8_t)root->symbol;
        while (decoded < orig_size) {
            int bit;
            /* Consume the bit (it should always be 0, but we must read it
             * to advance past any padding check). */
            if (bitreader_read_bit(&br, &bit) != 0) {
                fprintf(stderr,"decompress: unexpected eof (single-leaf) "
                        "at %llu/%llu\n",
                        (unsigned long long)decoded,
                        (unsigned long long)orig_size);
                free(out_buf); huffman_free_tree(root); return -1;
            }
            out_buf[buf_pos++] = sym;
            decoded++;
            if (buf_pos == IO_BUF_SIZE) {
                if (fwrite(out_buf,1,buf_pos,dst) != buf_pos) {
                    fputs("decompress: write error\n",stderr);
                    free(out_buf); huffman_free_tree(root); return -1;
                }
                buf_pos = 0;
            }
        }
    } else {
        /* General case: traverse tree per bit until we hit a leaf. */
        HuffNode *node = root;
        while (decoded < orig_size) {
            int bit;
            if (bitreader_read_bit(&br, &bit) != 0) {
                fprintf(stderr,"decompress: unexpected eof at %llu/%llu\n",
                        (unsigned long long)decoded,
                        (unsigned long long)orig_size);
                free(out_buf); huffman_free_tree(root); return -1;
            }
            node = bit ? node->right : node->left;
            if (!node) {
                fputs("decompress: corrupt bitstream\n",stderr);
                free(out_buf); huffman_free_tree(root); return -1;
            }
            if (!node->left && !node->right) {
                out_buf[buf_pos++] = (uint8_t)node->symbol;
                decoded++;
                if (buf_pos == IO_BUF_SIZE) {
                    if (fwrite(out_buf,1,buf_pos,dst) != buf_pos) {
                        fputs("decompress: write error\n",stderr);
                        free(out_buf); huffman_free_tree(root); return -1;
                    }
                    buf_pos = 0;
                }
                node = root;
            }
        }
    }

    /* Flush remaining output buffer */
    if (buf_pos > 0) {
        if (fwrite(out_buf,1,buf_pos,dst) != buf_pos) {
            fputs("decompress: write error\n",stderr);
            free(out_buf); huffman_free_tree(root); return -1;
        }
    }

    free(out_buf);
    huffman_free_tree(root);
    return 0;
}
