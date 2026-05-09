#pragma once
// base64.h — Base64 encode/decode for CrossPoint Android port
// Replaces the empty stub with a full implementation
// API matches the ESP32 Arduino base64 class

#include "WString.h"
#include <cstddef>
#include <cstdint>

class base64 {
public:
    // Encode binary data to base64 string
    static String encode(const uint8_t* data, size_t length) {
        static const char base64chars[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        if (!data || length == 0) return String();

        String result;
        size_t i = 0;

        while (i < length) {
            uint32_t octet_a = data[i++];
            uint32_t octet_b = (i < length) ? data[i++] : 0;
            uint32_t octet_c = (i <= length) ? ((i - 1 < length) ? data[i - 1] : 0) : 0;

            // Recalculate properly
            i = (i > length) ? i : i; // no-op, just recalculate below
        }

        // Reset and use a cleaner approach
        result.clear();
        i = 0;

        while (i < length) {
            // How many bytes do we have in this group?
            size_t remaining = length - i;
            uint32_t b0 = data[i++];

            result += base64chars[(b0 >> 2) & 0x3F];

            if (remaining == 1) {
                // 1 byte → 2 base64 chars + "=="
                result += base64chars[(b0 << 4) & 0x30];
                result += '=';
                result += '=';
                break;
            }

            uint32_t b1 = data[i++];
            result += base64chars[((b0 << 4) | (b1 >> 4)) & 0x3F];

            if (remaining == 2) {
                // 2 bytes → 3 base64 chars + "="
                result += base64chars[(b1 << 2) & 0x3C];
                result += '=';
                break;
            }

            uint32_t b2 = data[i++];
            result += base64chars[((b1 << 2) | (b2 >> 6)) & 0x3F];
            result += base64chars[b2 & 0x3F];
            // 3 bytes → 4 base64 chars, no padding
        }

        return result;
    }

    // Encode a String to base64
    static String encode(const String& text) {
        return encode(reinterpret_cast<const uint8_t*>(text.c_str()), text.length());
    }

    // Decode a base64 string to binary output
    static size_t decode(const char* input, size_t inputLen, uint8_t* output, size_t outCapacity) {
        static const uint8_t d[] = {
            62, 255, 255, 255, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 255,
            255, 255, 255, 255, 255, 255, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
            10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
            255, 255, 255, 255, 255, 255, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
            36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51
        };

        if (!input || !output) return 0;

        size_t outIdx = 0;
        uint32_t buf = 0;
        int bits = 0;

        for (size_t i = 0; i < inputLen; i++) {
            char c = input[i];
            if (c == '=' || c == '\0') break;
            if (c < '+' || c > 'z') continue;

            uint8_t val = d[c - '+'];
            if (val == 255) continue;

            buf = (buf << 6) | val;
            bits += 6;

            if (bits >= 8) {
                bits -= 8;
                if (outIdx < outCapacity) {
                    output[outIdx++] = (uint8_t)(buf >> bits);
                }
            }
        }

        return outIdx;
    }

    // Decode a base64 String to binary output
    static size_t decode(const String& input, uint8_t* output, size_t outCapacity) {
        return decode(input.c_str(), input.length(), output, outCapacity);
    }

    // Calculate expected encoded length
    static size_t encodeExpectedLen(size_t inputLen) {
        return 4 * ((inputLen + 2) / 3);
    }
};