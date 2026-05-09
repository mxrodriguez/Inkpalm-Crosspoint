#pragma once

#include <android/log.h>
#include <string>
#include <cstring>
#include <cstdio>
#include <cerrno>

#ifndef LOG_LEVEL
#define LOG_LEVEL 0
#endif

#define RTC_NOINIT_ATTR
#define RTC_DATA_ATTR
#define ENABLE_SERIAL_LOG

#define LOG_ERR(o, f, ...) __android_log_print(ANDROID_LOG_ERROR, o, f, ##__VA_ARGS__)
#define LOG_INF(o, f, ...) __android_log_print(ANDROID_LOG_INFO, o, f, ##__VA_ARGS__)
#define LOG_DBG(o, f, ...) ((void)0)

void logPrintf(const char* level, const char* origin, const char* format, ...);
std::string getLastLogs();
void clearLastLogs();
bool sanitizeLogHead();

#define Serial ((void)0)