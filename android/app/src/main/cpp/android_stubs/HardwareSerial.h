#pragma once
#include <cstdio>

class HWCDC {
public:
    void begin(unsigned long) {}
    size_t write(uint8_t) { return 1; }
    size_t write(const char*) { return 1; }
    void print(const char*) {}
    void printf(const char*, ...) {}
    void flush() {}
    operator bool() const { return true; }
    bool available() { return false; }
    int read() { return -1; }
};

class HardwareSerial {
public:
    void begin(unsigned long) {}
    size_t write(uint8_t) { return 1; }
    size_t write(const char*) { return 1; }
    void print(const char*) {}
    void printf(const char*, ...) {}
    void flush() {}
    operator bool() const { return true; }
    bool available() { return false; }
    int read() { return -1; }
};

class Print {
public:
    virtual size_t write(uint8_t) { return 1; }
    virtual size_t write(const uint8_t*, size_t) { return 1; }
    size_t print(const char* s) { while (*s) write((uint8_t)*s++); return 1; }
    size_t println(const char* s) { print(s); write('\n'); return 1; }
};

extern HWCDC Serial;