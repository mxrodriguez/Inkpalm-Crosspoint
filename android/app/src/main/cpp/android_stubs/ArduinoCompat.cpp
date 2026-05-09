// ArduinoCompat.cpp — Implementation of Arduino compatibility classes
// This file provides runtime implementations that can't be inlined in headers

#include "Arduino.h"
#include "HardwareSerial.h"
#include "HalDisplay.h"
#include <android/log.h>
#include <chrono>
#include <unistd.h>

// ---- Print::printf implementation ----
size_t Print::printf(const char* fmt, ...) {
    char buf[512];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (len > 0) {
        return print(buf);
    }
    return 0;
}

// ---- Android Log Serial ----
class AndroidLogPrint : public Print {
public:
    size_t write(uint8_t b) override {
        char c = (char)b;
        __android_log_print(ANDROID_LOG_DEBUG, "CP-Serial", "%c", c);
        return 1;
    }
    size_t write(const uint8_t* buf, size_t len) override {
        // Truncate for logcat safety
        char tmp[400];
        size_t copyLen = len < sizeof(tmp) - 1 ? len : sizeof(tmp) - 1;
        memcpy(tmp, buf, copyLen);
        tmp[copyLen] = '\0';
        __android_log_print(ANDROID_LOG_DEBUG, "CP-Serial", "%s", tmp);
        return len;
    }
    size_t printf(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        __android_log_vprint(ANDROID_LOG_DEBUG, "CP-Serial", fmt, args);
        va_end(args);
        return 0;
    }
};

static AndroidLogPrint gAndroidLogSerial;
Print* SerialPtr = &gAndroidLogSerial;

// ---- Arduino function stubs ----
static auto gStartTime = std::chrono::steady_clock::now();

unsigned long millis() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - gStartTime);
    return (unsigned long)elapsed.count();
}

unsigned long micros() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - gStartTime);
    return (unsigned long)elapsed.count();
}

void delay(unsigned long ms) {
    usleep(ms * 1000);
}

void yield() {
    // On Android, this could call sched_yield() but it's rarely needed
}

void delayMicroseconds(unsigned int us) {
    usleep(us);
}

void pinMode(int, int) {}
void digitalWrite(int, int) {}
int digitalRead(int) { return 0; }

// ---- Global display instance ----
HalDisplay display;