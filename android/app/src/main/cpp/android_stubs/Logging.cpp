#include "Logging.h"
#include <cstring>

constexpr uint32_t LOG_RTC_MAGIC = 0xDEADBEEF;
constexpr size_t MAX_LOG_LINES = 16;
constexpr size_t MAX_ENTRY_LEN = 256;

char logMessages[MAX_LOG_LINES][MAX_ENTRY_LEN];
size_t logHead = 0;
uint32_t rtcLogMagic = LOG_RTC_MAGIC;

void addToLogRingBuffer(const char* message) {
    if (rtcLogMagic != LOG_RTC_MAGIC || logHead >= MAX_LOG_LINES) {
        memset(logMessages, 0, sizeof(logMessages));
        logHead = 0;
        rtcLogMagic = LOG_RTC_MAGIC;
    }
    strncpy(logMessages[logHead], message, MAX_ENTRY_LEN - 1);
    logMessages[logHead][MAX_ENTRY_LEN - 1] = '\0';
    logHead = (logHead + 1) % MAX_LOG_LINES;
}

void logPrintf(const char* level, const char* origin, const char* format, ...) {
    char buf[MAX_ENTRY_LEN];
    int len = snprintf(buf, sizeof(buf), "[0] [%s] [%s] ", level, origin);
    if (len > 0 && len < (int)MAX_ENTRY_LEN) {
        __android_log_print(ANDROID_LOG_INFO, origin, "%s", buf);
    }
    addToLogRingBuffer(buf);
}

std::string getLastLogs() {
    if (rtcLogMagic != LOG_RTC_MAGIC) return {};
    std::string output;
    for (size_t i = 0; i < MAX_LOG_LINES; i++) {
        size_t idx = (logHead + i) % MAX_LOG_LINES;
        if (logMessages[idx][0] != '\0') {
            output.append(logMessages[idx]);
        }
    }
    return output;
}

bool sanitizeLogHead() {
    if (rtcLogMagic != LOG_RTC_MAGIC || logHead >= MAX_LOG_LINES) {
        logHead = 0;
        return true;
    }
    return false;
}

void clearLastLogs() {
    for (size_t i = 0; i < MAX_LOG_LINES; i++) logMessages[i][0] = '\0';
    logHead = 0;
    rtcLogMagic = LOG_RTC_MAGIC;
}