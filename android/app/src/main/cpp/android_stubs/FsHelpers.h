#pragma once
#include "WString.h"
#include "HalStorage.h"

namespace FsHelpers {
    String getBaseName(const String& path);
    String getDirName(const String& path);
    String combinePath(const String& base, const String& name);
    bool isHiddenFile(const String& filename);
    String getFileExtension(const String& filename);
    String getFileNameWithoutExtension(const String& filename);
    bool fileExists(const String& path);
    uint32_t getFileSize(const String& path);
}