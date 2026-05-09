#pragma once
// Stub for mbedtls/base64.h — redirects to our own base64 implementation
// Matches the mbedtls API used by ObfuscationUtils.cpp

#include "base64.h"
#include <cstddef>
#include <cstdint>
#include <cstring>

#define MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL -0x0010

inline int mbedtls_base64_encode(unsigned char* dst, size_t dlen, size_t* olen,
                                 const unsigned char* src, size_t slen) {
    size_t needed = base64::encodeExpectedLen(slen);
    if (dst == nullptr || dlen < needed) {
        *olen = needed;
        return MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL;
    }
    std::string encoded = base64::encode(src, slen);
    *olen = encoded.length();
    memcpy(dst, encoded.c_str(), encoded.length());
    return 0;
}

inline int mbedtls_base64_decode(unsigned char* dst, size_t dlen, size_t* olen,
                                 const unsigned char* src, size_t slen) {
    // Approximate max decoded size: slen * 3/4 (may be slightly over, safe for buffer sizing)
    size_t needed = (slen / 4) * 3 + 3;
    if (dst == nullptr || dlen < needed) {
        *olen = needed;
        return MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL;
    }
    *olen = base64::decode(reinterpret_cast<const char*>(src), slen, dst, dlen);
    return 0;
}
