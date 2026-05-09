#pragma once
#include <string>
class String : public std::string {
public:
    String() : std::string() {}
    String(const char* s) : std::string(s) {}
    String(const std::string& s) : std::string(s) {}
    int indexOf(char c, int from = 0) const {
        size_t pos = find(c, from);
        return pos == npos ? -1 : (int)pos;
    }
    int indexOf(const String& s, int from = 0) const {
        size_t pos = find(s, from);
        return pos == npos ? -1 : (int)pos;
    }
    String substring(int start) const { return substr(start); }
    String substring(int start, int len) const { return substr(start, len); }
    int length() const { return (int)size(); }
    bool startsWith(const String& s) const { return find(s) == 0; }
    bool endsWith(const String& s) const { return rfind(s) == size() - s.size(); }
    String trim() const;
    void toLowerCase();
    void toUpperCase();
    int lastIndexOf(char c) const {
        size_t pos = rfind(c);
        return pos == npos ? -1 : (int)pos;
    }
    String replace(int start, int len, const String& with) const;
};

String format(const char* fmt, ...);

#define FPSTR(p) ((const char*)(p))
#define FSTRING String