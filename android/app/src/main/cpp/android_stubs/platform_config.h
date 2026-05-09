#pragma once
#include <cstdint>
#include <cstring>
#include <string>

#define RTC_NOINIT_ATTR
#define RTC_DATA_ATTR

constexpr uint32_t LOG_RTC_MAGIC = 0xDEADBEEF;
inline unsigned long millis() { return 0; }
inline void delay(unsigned long) {}
inline void yield() {}