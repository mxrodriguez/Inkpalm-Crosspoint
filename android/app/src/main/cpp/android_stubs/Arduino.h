#pragma once
// Arduino compatibility header for CrossPoint Android port
// Consolidates Print + String + Arduino function stubs
// No ODR violations: Print is defined here only, String is in WString.h

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
    size_t println(const char* s) { size_t n = print(s); n += write('\n'); return n; }
    size_t println(const String& s) { return println(s.c_str()); }
    size_t println() { return write('\n'); }

    // printf support for logging
    size_t printf(const char* fmt, ...) __attribute__((format(printf, 2, 3)));
};

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