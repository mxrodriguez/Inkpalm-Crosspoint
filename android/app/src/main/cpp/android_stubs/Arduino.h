#pragma once
#include <cstdint>
#include <cstring>

class Print {
public:
    virtual size_t write(uint8_t) { return 1; }
    virtual size_t write(const uint8_t* buf, size_t len) { (void)buf; (void)len; return len; }
    size_t print(const char* s) { while (*s) write((uint8_t)*s++); return 1; }
    size_t println(const char* s) { print(s); write('\n'); return 1; }
};

class String {
public:
    String() : buffer(nullptr), len(0), capacity(0) {}
    String(const char* s) {
        if (s) {
            len = strlen(s);
            capacity = len + 1;
            buffer = new char[capacity];
            memcpy(buffer, s, len + 1);
        } else {
            buffer = nullptr; len = capacity = 0;
        }
    }
    String(const String& s) {
        len = s.len; capacity = s.capacity;
        buffer = new char[capacity];
        memcpy(buffer, s.buffer, len + 1);
    }
    ~String() { if (buffer) delete[] buffer; }
    String& operator=(const char* s) {
        if (s) {
            len = strlen(s);
            if (capacity < len + 1) {
                delete[] buffer;
                capacity = len + 1;
                buffer = new char[capacity];
            }
            memcpy(buffer, s, len + 1);
        } else { len = 0; if (buffer) buffer[0] = 0; }
        return *this;
    }
    const char* c_str() const { return buffer ? buffer : ""; }
    size_t length() const { return len; }
    bool empty() const { return len == 0; }
private:
    char* buffer = nullptr;
    size_t len = 0;
    size_t capacity = 0;
};

void pinMode(int, int) {}
void digitalWrite(int, int) {}
int digitalRead(int) { return 0; }
unsigned long millis() { return 0; }
void delay(unsigned long) {}
void yield() {}
unsigned long micros() { return 0; }
void delayMicroseconds(unsigned int) {}

extern Print Serial;