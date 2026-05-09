#include "FsHelpers.h"
#include <cstring>
#include <algorithm>

namespace FsHelpers {
    String getBaseName(const String& path) {
        int sep = path.lastIndexOf('/');
        if (sep < 0) sep = path.lastIndexOf('\\');
        return sep >= 0 ? path.substring(sep + 1) : path;
    }

    String getDirName(const String& path) {
        int sep = path.lastIndexOf('/');
        if (sep < 0) sep = path.lastIndexOf('\\');
        return sep >= 0 ? path.substring(0, sep) : String("");
    }

    String combinePath(const String& base, const String& name) {
        if (base.empty()) return name;
        char last = base[base.length() - 1];
        return base + (last == '/' || last == '\\' ? "" : "/") + name;
    }

    bool isHiddenFile(const String& filename) {
        return filename.startsWith(".");
    }

    String getFileExtension(const String& filename) {
        int dot = filename.lastIndexOf('.');
        return dot >= 0 ? filename.substring(dot + 1).toLowerCase() : String("");
    }

    String getFileNameWithoutExtension(const String& filename) {
        int dot = filename.lastIndexOf('.');
        return dot >= 0 ? filename.substring(0, dot) : filename;
    }

    bool fileExists(const String& path) {
        FILE* f = fopen(path.c_str(), "rb");
        if (f) { fclose(f); return true; }
        return false;
    }

    uint32_t getFileSize(const String& path) {
        FILE* f = fopen(path.c_str(), "rb");
        if (!f) return 0;
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        fclose(f);
        return (uint32_t)sz;
    }
}