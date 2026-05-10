#pragma once
// Arduino compatibility header for CrossPoint Android port
// SINGLE TRUTH SOURCE for all Arduino-compat types.
// Always include this header; never include WString.h or Print.h directly.
// No ODR violations: Print is defined here, String is in WString.h (included below).

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

// Forward declare String so Print can reference it
#include "WString.h"

// ---- Print class (single definition, no duplicates) ----
class Print {
public:
    virtual ~Print() = default;
    virtual size_t write(uint8_t b) { (void)b; return 1; }
    virtual size_t write(const uint8_t* buf, size_t len) { (void)buf; (void)len; return len; }

    size_t print(const char* s) { size_t n = 0; while (*s) n += write((uint8_t)*s++); return n; }
    size_t print(const String& s) { return print(s.c_str()); }
    size_t print(int n) {
        char buf[16];
        int len = snprintf(buf, sizeof(buf), "%d", n);
        return write((const uint8_t*)buf, len);
    }
    size_t print(unsigned long n) {
        char buf[32];
        int len = snprintf(buf, sizeof(buf), "%lu", n);
        return write((const uint8_t*)buf, len);
    }
    size_t print(double n, int precision = 2) {
        char buf[64];
        int len = snprintf(buf, sizeof(buf), "%.*f", precision, n);
        return write((const uint8_t*)buf, len);
    }
    size_t print(char c) {
        return write((uint8_t)c);
    }
    size_t println(const char* s) { size_t n = print(s); n += write('\n'); return n; }
    size_t println(const String& s) { return println(s.c_str()); }
    size_t println() { return write('\n'); }
    size_t println(int n) {
        size_t sz = print(n);
        sz += write('\n');
        return sz;
    }
    size_t println(unsigned long n) {
        size_t sz = print(n);
        sz += write('\n');
        return sz;
    }
    size_t println(double n, int precision = 2) {
        size_t sz = print(n, precision);
        sz += write('\n');
        return sz;
    }
    size_t println(char c) {
        size_t sz = print(c);
        sz += write('\n');
        return sz;
    }

    // printf support for logging
    size_t printf(const char* fmt, ...) __attribute__((format(printf, 2, 3)));
};

// ---- ESP stubs (ESP.getFreeHeap(), etc.) ----
#include "ESP.h"

// ---- Arduino function stubs ----
#ifndef CROSSPOINT_ANDROID
#define CROSSPOINT_ANDROID 1
#endif

void pinMode(int, int);
void digitalWrite(int, int);
int digitalRead(int);
unsigned long millis();
void delay(unsigned long);
void yield();
unsigned long micros();
void delayMicroseconds(unsigned int);

// Global Serial — pointer to Print that logs to Android logcat
// Initialized in ArduinoCompat.cpp
extern Print* SerialPtr;
#define Serial (*SerialPtr)