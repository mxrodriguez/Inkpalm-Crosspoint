#pragma once
// platform_config.h — Platform-specific defines for CrossPoint Android port

#include <cstdint>

// ESP32 RTC memory attributes — not applicable on Android
#define RTC_NOINIT_ATTR
#define RTC_DATA_ATTR

// Magic value for log ring buffer initialization
constexpr uint32_t LOG_RTC_MAGIC = 0xDEADBEEF;