#pragma once
#include <cstdio>

class Print {
public:
    virtual size_t write(uint8_t) { return 1; }
    virtual size_t write(const uint8_t* buf, size_t len) { (void)buf; (void)len; return len; }
    size_t print(const char* s) { while (*s) write((uint8_t)*s++); return 1; }
    size_t println(const char* s) { print(s); write('\n'); return 1; }
};