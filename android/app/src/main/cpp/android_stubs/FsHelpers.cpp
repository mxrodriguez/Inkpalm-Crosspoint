#include "FsHelpers.h"
#include <cctype>
#include <cstring>
#include <algorithm>
#include <string>
#include <vector>

namespace FsHelpers {

std::string normalisePath(const std::string& path) {
    std::vector<std::string> components;
    std::string component;

    for (const auto c : path) {
        if (c == '/') {
            if (!component.empty()) {
                if (component == "..") {
                    if (!components.empty()) {
                        components.pop_back();
                    }
                } else {
                    components.push_back(component);
                }
                component.clear();
            }
        } else {
            component += c;
        }
    }

    if (!component.empty()) {
        components.push_back(component);
    }

    std::string result;
    for (const auto& c : components) {
        if (!result.empty()) {
            result += "/";
        }
        result += c;
    }
    return result;
}

void sortFileList(std::vector<std::string>& strs) {
    std::sort(begin(strs), end(strs), [](const std::string& a, const std::string& b) {
        bool isDir1 = a.back() == '/';
        bool isDir2 = b.back() == '/';
        if (isDir1 != isDir2) return isDir1;
        return a < b;
    });
}

bool checkFileExtension(std::string_view fileName, const char* extension) {
    auto dot = fileName.rfind('.');
    if (dot == std::string_view::npos) return false;
    std::string_view ext = fileName.substr(dot + 1);
    if (ext.size() != strlen(extension)) return false;
    for (size_t i = 0; i < ext.size(); ++i) {
        if (std::tolower(ext[i]) != std::tolower(extension[i])) return false;
    }
    return true;
}

bool hasJpgExtension(std::string_view fileName) {
    return checkFileExtension(fileName, "jpg") || checkFileExtension(fileName, "jpeg");
}

bool hasPngExtension(std::string_view fileName) {
    return checkFileExtension(fileName, "png");
}

bool hasBmpExtension(std::string_view fileName) {
    return checkFileExtension(fileName, "bmp");
}

bool hasGifExtension(std::string_view fileName) {
    return checkFileExtension(fileName, "gif");
}

bool hasEpubExtension(std::string_view fileName) {
    return checkFileExtension(fileName, "epub");
}

bool hasXtcExtension(std::string_view fileName) {
    return checkFileExtension(fileName, "xtc") || checkFileExtension(fileName, "xtch");
}

bool hasTxtExtension(std::string_view fileName) {
    return checkFileExtension(fileName, "txt");
}

bool hasMarkdownExtension(std::string_view fileName) {
    return checkFileExtension(fileName, "md");
}

std::string extractFolderPath(const std::string& filePath) {
    auto pos = filePath.rfind('/');
    if (pos == std::string::npos) return "";
    return filePath.substr(0, pos + 1);
}

void sanitizePathComponentForFat32(const char* input, char* output, size_t maxLen) {
    size_t i = 0;
    for (size_t j = 0; input[j] != '\0' && i < maxLen - 1; ++j) {
        unsigned char c = static_cast<unsigned char>(input[j]);
        if (c <= 0x1F || c == 0x7F || c == '/' || c == '\\' || c == ':' || c == '*' ||
            c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
            output[i++] = '-';
        } else {
            output[i++] = c;
        }
    }
    output[i] = '\0';
}

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
        if (dot < 0) return String("");
        String ext = filename.substring(dot + 1);
        ext.toLowerCase();
        return ext;
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