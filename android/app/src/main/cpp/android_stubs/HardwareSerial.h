#pragma once
// HardwareSerial.h — Serial compatibility for CrossPoint Android port
// On Android, serial output goes to Android logcat

#include "Arduino.h"  // Pulls in Print, String, and Arduino stubs

// HWCDC — USB CDC serial (maps to logcat on Android)
class HWCDC : public Print {
public:
    void begin(unsigned long) {}
    size_t write(uint8_t b) override { (void)b; return 1; }
    size_t write(const uint8_t* buf, size_t len) override { (void)buf; (void)len; return len; }
    void print(const char* s) { (void)s; }
    void printf(const char* fmt, ...) __attribute__((format(printf, 2, 3))) {
        va_list args;
        va_start(args, fmt);
        va_end(args);
    }
    void flush() {}
    operator bool() const { return true; }
    bool available() { return false; }
    int read() { return -1; }
};

// HardwareSerial — UART serial (maps to logcat on Android)
class HardwareSerial : public Print {
public:
    void begin(unsigned long) {}
    size_t write(uint8_t b) override { (void)b; return 1; }
    size_t write(const uint8_t* buf, size_t len) override { (void)buf; (void)len; return len; }
    void print(const char* s) { (void)s; }
    void printf(const char* fmt, ...) __attribute__((format(printf, 2, 3))) {
        va_list args;
        va_start(args, fmt);
        va_end(args);
    }
    void flush() {}
    operator bool() const { return true; }
    bool available() { return false; }
    int read() { return -1; }
};

// Serial is defined in Arduino.h as a macro (*SerialPtr)
// On Android, SerialPtr points to an AndroidLogPrint that writes to logcat