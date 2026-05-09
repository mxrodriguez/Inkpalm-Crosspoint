/*
 * uzlib_checksums.c — Real Adler-32 and CRC-32 implementations for uzlib
 * Ported from upstream uzlib (https://github.com/pfalcon/uzlib)
 * These replace the no-op stubs that would cause ZIP decompression failures
 */

#include "uzlib.h"

/* ---- Adler-32 checksum (RFC 1950) ---- */

#define A32_BASE 65521
#define A32_NMAX 5552

uint32_t TINFCC uzlib_adler32(const void *data, unsigned int length, uint32_t prev_sum)
{
    const unsigned char *buf = (const unsigned char *)data;
    unsigned int s1 = prev_sum & 0xffff;
    unsigned int s2 = prev_sum >> 16;

    while (length > 0) {
        int k = length < A32_NMAX ? length : A32_NMAX;
        int i;

        /* Process 16 bytes at a time for speed */
        for (i = k / 16; i; --i, buf += 16) {
            s1 += buf[0];  s2 += s1;
            s1 += buf[1];  s2 += s1;
            s1 += buf[2];  s2 += s1;
            s1 += buf[3];  s2 += s1;
            s1 += buf[4];  s2 += s1;
            s1 += buf[5];  s2 += s1;
            s1 += buf[6];  s2 += s1;
            s1 += buf[7];  s2 += s1;
            s1 += buf[8];  s2 += s1;
            s1 += buf[9];  s2 += s1;
            s1 += buf[10]; s2 += s1;
            s1 += buf[11]; s2 += s1;
            s1 += buf[12]; s2 += s1;
            s1 += buf[13]; s2 += s1;
            s1 += buf[14]; s2 += s1;
            s1 += buf[15]; s2 += s1;
        }

        /* Process remaining bytes */
        for (i = k % 16; i; --i) {
            s1 += *buf++;
            s2 += s1;
        }

        s1 %= A32_BASE;
        s2 %= A32_BASE;
        length -= k;
    }

    return ((uint32_t)s2 << 16) | s1;
}

/* ---- CRC-32 checksum (ISO 3309 / ITU-T V.42) ---- */

static const uint32_t tinf_crc32tab[16] = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
};

uint32_t TINFCC uzlib_crc32(const void *data, unsigned int length, uint32_t crc)
{
    const unsigned char *buf = (const unsigned char *)data;
    unsigned int i;

    for (i = 0; i < length; ++i) {
        crc ^= buf[i];
        crc = tinf_crc32tab[crc & 0x0f] ^ (crc >> 4);
        crc = tinf_crc32tab[crc & 0x0f] ^ (crc >> 4);
    }

    return crc;
}