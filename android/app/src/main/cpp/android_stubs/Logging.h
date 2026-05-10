#pragma once
// Logging.h — CrossPoint Android logging compatibility
// Uses Android logcat for all log output

#include <android/log.h>
#include <string>
#include <cstring>
#include <cstdio>
#include <cerrno>

#ifndef LOG_LEVEL
#define LOG_LEVEL 0
#endif

// RTC attributes are no-ops on Android (defined in platform_config.h)
#ifndef RTC_NOINIT_ATTR
#define RTC_NOINIT_ATTR
#endif
#ifndef RTC_DATA_ATTR
#define RTC_DATA_ATTR
#endif

#define ENABLE_SERIAL_LOG

#define LOG_ERR(o, f, ...) __android_log_print(ANDROID_LOG_ERROR, o, f, ##__VA_ARGS__)
#define LOG_WRN(o, f, ...) __android_log_print(ANDROID_LOG_WARN, o, f, ##__VA_ARGS__)
#define LOG_INF(o, f, ...) __android_log_print(ANDROID_LOG_INFO, o, f, ##__VA_ARGS__)
#define LOG_DBG(o, f, ...) ((void)0)

// Log ring buffer functions (used by CrossPoint core for crash logs)
void logPrintf(const char* level, const char* origin, const char* format, ...);
std::string getLastLogs();
void clearLastLogs();
bool sanitizeLogHead();

// Serial is defined in Arduino.h — don't redefine here
// If you need Serial without including Arduino.h, include this header
// and Arduino.h will provide the actual Serial object