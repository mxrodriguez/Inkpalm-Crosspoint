#pragma once
// Stub for esp_mac.h — not available on Android
// Provides a fake MAC address for ObfuscationUtils

#include <cstdint>
#include <cstring>

inline void esp_efuse_mac_get_default(uint8_t* mac) {
    // Use a fixed "Android" MAC address for obfuscation key derivation
    // This means obfuscated credentials from ESP32 won't transfer,
    // but that's expected — the Android app will re-enter credentials.
    const uint8_t defaultMac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
    memcpy(mac, defaultMac, 6);
}
