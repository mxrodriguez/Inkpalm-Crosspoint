#pragma once

#include <cstdio>
#include <sys/stat.h>
#include <cerrno>
#include <cstring>
#include <string>
#include <Print.h>

#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2
#define O_CREAT 0x40
#define O_TRUNC 0x200

using oflag_t = int;

namespace fs {

class HalFile : public Print {
public:
    HalFile() : file(nullptr) {}
    ~HalFile() { close(); }

    bool open(const char* path, const char* mode) {
        if (file) fclose(file);
        file = fopen(path, mode);
        return file != nullptr;
    }
    void close() { if (file) { fclose(file); file = nullptr; } }
    bool isOpen() const { return file != nullptr; }
    bool seek(size_t pos) { return file ? fseek(file, (long)pos, SEEK_SET) == 0 : false; }
    bool seekCur(int64_t offset) { return file ? fseek(file, (long)offset, SEEK_CUR) == 0 : false; }
    int available() const {
        if (!file) return 0;
        long pos = ftell(file);
        fseek(file, 0, SEEK_END);
        long end = ftell(file);
        fseek(file, pos, SEEK_SET);
        return (int)(end - pos);
    }
    size_t position() const { return file ? (size_t)ftell(file) : 0; }
    size_t size() {
        if (!file) return 0;
        long pos = ftell(file);
        fseek(file, 0, SEEK_END);
        long sz = ftell(file);
        fseek(file, pos, SEEK_SET);
        return (size_t)sz;
    }
    int read(void* buf, size_t count) { return file ? (int)fread(buf, 1, count, file) : -1; }
    size_t write(const void* buf, size_t count) { return file ? fwrite(buf, 1, count, file) : 0; }
    void flush() { if (file) fflush(file); }
    bool rename(const char*) { return false; }
    bool isDirectory() const { return false; }
    void rewindDirectory() {}
    HalFile openNextFile() { return HalFile(); }
    operator bool() const { return isOpen(); }
    int read() {
        unsigned char c;
        return file && fread(&c, 1, 1, file) == 1 ? c : -1;
    }
    size_t write(uint8_t b) override { return file ? fwrite(&b, 1, 1, file) : 0; }

private:
    FILE* file;
};

class HalStorage {
public:
    bool begin() { return true; }
    bool ready() const { return true; }
    bool openFileForRead(const char*, const std::string& path, HalFile& file) {
        file.open(path.c_str(), "rb"); return file.isOpen();
    }
    HalFile open(const char* path, int oflag = O_RDONLY) {
        HalFile f;
        const char* mode = (oflag == O_WRONLY || oflag == O_CREAT) ? "wb" : "rb";
        f.open(path, mode); return f;
    }
    bool exists(const char* path) { struct stat st; return stat(path, &st) == 0; }
    bool remove(const char* path) { return ::remove(path) == 0; }
    bool mkdir(const char* path, bool = true) { return ::mkdir(path, 0755) == 0 || errno == EEXIST; }
    static HalStorage& getInstance() { return instance; }

private:
    static HalStorage instance;
};

// Define the static instance
inline HalStorage HalStorage::instance;

} // namespace fs

using FsFile = fs::HalFile;
#define Storage fs::HalStorage::getInstance()