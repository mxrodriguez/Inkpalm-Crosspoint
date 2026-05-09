#pragma once
// WString.h — Arduino String compatibility for CrossPoint Android port
// Extends std::string for minimal memory overhead while providing Arduino API

#include <string>
#include <cctype>
#include <algorithm>
#include <cstdio>
#include <cstdarg>
#include <cstring>

class String : public std::string {
public:
    // Constructors
    String() : std::string() {}
    String(const char* s) : std::string(s ? s : "") {}
    String(const std::string& s) : std::string(s) {}
    String(const String& s) : std::string(s) {}

    // Arduino API — search
    int indexOf(char c, int from = 0) const {
        size_t pos = find(c, from);
        return pos == npos ? -1 : (int)pos;
    }
    int indexOf(const String& s, int from = 0) const {
        size_t pos = find(s, from);
        return pos == npos ? -1 : (int)pos;
    }
    int lastIndexOf(char c) const {
        size_t pos = rfind(c);
        return pos == npos ? -1 : (int)pos;
    }
    int lastIndexOf(const String& s) const {
        size_t pos = rfind(s);
        return pos == npos ? -1 : (int)pos;
    }

    // Arduino API — substring (from, to) where 'to' is END POSITION, not length
    String substring(int start) const { return substr(start); }
    String substring(int start, int end) const {
        if (end <= start) return String();
        return substr(start, end - start);
    }

    // Arduino API — length (int version)
    int length() const { return (int)size(); }
    bool empty() const { return std::string::empty(); }

    // Arduino API — prefix/suffix
    bool startsWith(const String& s) const { return find(s) == 0; }
    bool endsWith(const String& s) const {
        if (s.size() > size()) return false;
        return compare(size() - s.size(), s.size(), s) == 0;
    }

    // Arduino API — trim (remove leading/trailing whitespace)
    String trim() const {
        size_t start = find_first_not_of(" \t\n\r\f\v");
        if (start == npos) return String();
        size_t end = find_last_not_of(" \t\n\r\f\v");
        return substr(start, end - start + 1);
    }

    // Arduino API — case conversion (in-place)
    void toLowerCase() {
        for (auto& c : *this) c = (char)tolower((unsigned char)c);
    }

    void toUpperCase() {
        for (auto& c : *this) c = (char)toupper((unsigned char)c);
    }

    // Arduino API — replace (2-arg: replace all occurrences of 'from' with 'with')
    void replace(const String& from, const String& with) {
        if (from.empty()) return;
        size_t pos = 0;
        while ((pos = find(from, pos)) != npos) {
            std::string::replace(pos, from.size(), with);
            pos += with.size();
        }
    }

    // Arduino API — remove (delete chars from position)
    void remove(int index, int count = -1) {
        if (count < 0) {
            erase(index);
        } else {
            erase(index, count);
        }
    }

    // Arduino API — c_str (inherited from std::string)
    using std::string::c_str;

    // Arduino API — operator == with const char*
    bool operator==(const char* s) const { return compare(s) == 0; }
    bool operator!=(const char* s) const { return compare(s) != 0; }

    // Arduino API — concatenation
    String& operator+=(char c) { push_back(c); return *this; }
    String& operator+=(const char* s) { append(s); return *this; }
    String& operator+=(const String& s) { append(s); return *this; }

    // Arduino API — toInt, toFloat
    long toInt() const { return strtol(c_str(), nullptr, 10); }
    float toFloat() const { return strtof(c_str(), nullptr); }
};

// Format function (Arduino-style String formatting)
inline String format(const char* fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    return String(buf);
}

// Arduino flash-string helpers
#define FPSTR(p) ((const char*)(p))
#define FSTRING String