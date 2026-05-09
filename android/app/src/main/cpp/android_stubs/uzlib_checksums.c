#include "uzlib.h"

// Stub implementations for uzlib checksum functions
// These are not used in the Android EPUB reader implementation

uint32_t TINFCC uzlib_adler32(const void *data, unsigned int length, uint32_t prev_sum) {
    (void)data;
    (void)length;
    return prev_sum;
}

uint32_t TINFCC uzlib_crc32(const void *data, unsigned int length, uint32_t crc) {
    (void)data;
    (void)length;
    return crc;
}
