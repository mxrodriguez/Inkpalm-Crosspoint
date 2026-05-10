#pragma once
#include "Arduino.h"  // single truth source for String
#include "HalStorage.h"

#include <string>
#include <string_view>
#include <vector>

namespace FsHelpers {
    std::string normalisePath(const std::string& path);

    void sortFileList(std::vector<std::string>& strs);

    bool checkFileExtension(std::string_view fileName, const char* extension);
    bool hasJpgExtension(std::string_view fileName);
    bool hasPngExtension(std::string_view fileName);
    bool hasBmpExtension(std::string_view fileName);
    bool hasGifExtension(std::string_view fileName);
    bool hasEpubExtension(std::string_view fileName);
    bool hasXtcExtension(std::string_view fileName);
    bool hasTxtExtension(std::string_view fileName);
    bool hasMarkdownExtension(std::string_view fileName);

    std::string extractFolderPath(const std::string& filePath);

    void sanitizePathComponentForFat32(const char* input, char* output, size_t maxLen);

    String getBaseName(const String& path);
    String getDirName(const String& path);
    String combinePath(const String& base, const String& name);
    bool isHiddenFile(const String& filename);
    String getFileExtension(const String& filename);
    String getFileNameWithoutExtension(const String& filename);
    bool fileExists(const String& path);
    uint32_t getFileSize(const String& path);
}