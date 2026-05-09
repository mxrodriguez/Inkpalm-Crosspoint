#include <jni.h>
#include <android/log.h>
#include <android/bitmap.h>
#include <string>
#include <memory>
#include <vector>
#include <cstring>

#include <ZipFile.h>
#include <expat.h>

#define LOG_TAG "CrossPoint-JNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// EPUB state
static std::string gEpubPath;
static std::string gContentOpfPath;
static std::string gContentBasePath;
static std::vector<std::string> gSpineItems;
static std::string gTitle;
static std::string gAuthor;
static int gChapterCount = 0;

// Helper to read file from zip to string
static bool readZipFileToString(const std::string& zipPath, const std::string& filePath, std::string& out) {
    ZipFile zip(zipPath);
    if (!zip.open()) {
        LOGE("Failed to open zip: %s", zipPath.c_str());
        return false;
    }

    size_t size = 0;
    uint8_t* data = zip.readFileToMemory(filePath.c_str(), &size, true);
    zip.close();

    if (!data) {
        LOGE("Failed to read file from zip: %s", filePath.c_str());
        return false;
    }

    out = std::string(reinterpret_cast<char*>(data), size);
    free(data);
    return true;
}

// XML parser state for content.opf
struct OpfParserState {
    std::string currentTag;
    std::string title;
    std::string creator;
    std::string spineItem;
    std::vector<std::string> manifestItems;
    std::vector<std::string> spineRefs;
    bool inManifest = false;
    bool inSpine = false;
    bool inMetadata = false;
};

static void opfStartElement(void* userData, const XML_Char* name, const XML_Char** attrs) {
    auto* state = static_cast<OpfParserState*>(userData);
    state->currentTag = name;

    if (strcmp(name, "metadata") == 0) state->inMetadata = true;
    if (strcmp(name, "manifest") == 0) state->inManifest = true;
    if (strcmp(name, "spine") == 0) state->inSpine = true;

    if (state->inMetadata) {
        if (strcmp(name, "dc:title") == 0 || strcmp(name, "title") == 0) {
            // Will capture in character data
        }
        if (strcmp(name, "dc:creator") == 0 || strcmp(name, "creator") == 0) {
            // Will capture in character data
        }
    }

    if (state->inManifest && strcmp(name, "item") == 0) {
        std::string id, href, mediaType;
        for (int i = 0; attrs[i] && attrs[i + 1]; i += 2) {
            if (strcmp(attrs[i], "id") == 0) id = attrs[i + 1];
            if (strcmp(attrs[i], "href") == 0) href = attrs[i + 1];
            if (strcmp(attrs[i], "media-type") == 0) mediaType = attrs[i + 1];
        }
        if (!id.empty() && !href.empty()) {
            state->manifestItems.push_back(id + "|" + href);
        }
    }

    if (state->inSpine && strcmp(name, "itemref") == 0) {
        for (int i = 0; attrs[i] && attrs[i + 1]; i += 2) {
            if (strcmp(attrs[i], "idref") == 0) {
                state->spineRefs.push_back(attrs[i + 1]);
            }
        }
    }
}

static void opfEndElement(void* userData, const XML_Char* name) {
    auto* state = static_cast<OpfParserState*>(userData);
    if (strcmp(name, "metadata") == 0) state->inMetadata = false;
    if (strcmp(name, "manifest") == 0) state->inManifest = false;
    if (strcmp(name, "spine") == 0) state->inSpine = false;
    state->currentTag = "";
}

static void opfCharacterData(void* userData, const XML_Char* s, int len) {
    auto* state = static_cast<OpfParserState*>(userData);
    if (state->inMetadata) {
        std::string data(s, len);
        if (state->currentTag == "dc:title" || state->currentTag == "title") {
            state->title += data;
        }
        if (state->currentTag == "dc:creator" || state->currentTag == "creator") {
            state->creator += data;
        }
    }
}

// HTML-to-text extraction state
struct HtmlToTextState {
    std::string text;
    std::string title;
    bool inStyle = false;
    bool inScript = false;
    bool inHeading = false;
    bool needSpace = false;
    bool titleCaptured = false;
};

static void htmlStartElement(void* userData, const XML_Char* name, const XML_Char** attrs) {
    auto* state = static_cast<HtmlToTextState*>(userData);

    if (strcmp(name, "style") == 0) { state->inStyle = true; return; }
    if (strcmp(name, "script") == 0) { state->inScript = true; return; }
    if (state->inStyle || state->inScript) return;

    // Capture first heading as chapter title
    if (!state->titleCaptured && (strcmp(name, "h1") == 0 || strcmp(name, "h2") == 0 || strcmp(name, "h3") == 0)) {
        state->inHeading = true;
        return;
    }

    // Block-level elements that should add newlines
    const char* blockElements[] = {"p", "div", "h1", "h2", "h3", "h4", "h5", "h6", "li", "br", "hr", nullptr};
    for (int i = 0; blockElements[i]; i++) {
        if (strcmp(name, blockElements[i]) == 0) {
            if (!state->text.empty() && state->text.back() != '\n') {
                state->text += "\n";
            }
            return;
        }
    }
}

static void htmlEndElement(void* userData, const XML_Char* name) {
    auto* state = static_cast<HtmlToTextState*>(userData);

    if (strcmp(name, "style") == 0) { state->inStyle = false; return; }
    if (strcmp(name, "script") == 0) { state->inScript = false; return; }
    if (state->inStyle || state->inScript) return;

    // Capture heading as chapter title
    if (state->inHeading && (strcmp(name, "h1") == 0 || strcmp(name, "h2") == 0 || strcmp(name, "h3") == 0)) {
        state->inHeading = false;
        state->titleCaptured = true;
        // Trim whitespace from title
        while (!state->title.empty() && state->title.back() == ' ') {
            state->title.pop_back();
        }
        return;
    }

    const char* blockElements[] = {"p", "div", "h1", "h2", "h3", "h4", "h5", "h6", "li", "br", "hr", nullptr};
    for (int i = 0; blockElements[i]; i++) {
        if (strcmp(name, blockElements[i]) == 0) {
            if (!state->text.empty() && state->text.back() != '\n') {
                state->text += "\n";
            }
            return;
        }
    }
}

static void htmlCharacterData(void* userData, const XML_Char* s, int len) {
    auto* state = static_cast<HtmlToTextState*>(userData);
    if (state->inStyle || state->inScript) return;

    std::string data(s, len);

    // Replace HTML entities
    size_t pos = 0;
    while ((pos = data.find("&nbsp;")) != std::string::npos) data.replace(pos, 6, " ");
    while ((pos = data.find("&amp;")) != std::string::npos) data.replace(pos, 5, "&");
    while ((pos = data.find("&lt;")) != std::string::npos) data.replace(pos, 4, "<");
    while ((pos = data.find("&gt;")) != std::string::npos) data.replace(pos, 4, ">");
    while ((pos = data.find("&quot;")) != std::string::npos) data.replace(pos, 6, "\"");
    while ((pos = data.find("&#39;")) != std::string::npos) data.replace(pos, 5, "'");

    // Capture heading text as title
    if (state->inHeading && !state->titleCaptured) {
        state->title += data;
        return;
    }

    // Collapse whitespace
    std::string cleaned;
    bool lastWasSpace = false;
    for (char c : data) {
        if (c == '\n' || c == '\r' || c == '\t') c = ' ';
        if (c == ' ') {
            if (!lastWasSpace && !cleaned.empty()) {
                cleaned += ' ';
                lastWasSpace = true;
            }
        } else {
            cleaned += c;
            lastWasSpace = false;
        }
    }

    state->text += cleaned;
}

// Word wrapping function
static std::vector<std::string> wrapText(const std::string& text, int charsPerLine) {
    std::vector<std::string> lines;
    std::string currentLine;

    size_t pos = 0;
    while (pos < text.size()) {
        // Find next newline
        size_t newlinePos = text.find('\n', pos);
        if (newlinePos == std::string::npos) newlinePos = text.size();

        std::string paragraph = text.substr(pos, newlinePos - pos);
        pos = newlinePos + 1;

        // Skip empty lines
        if (paragraph.empty()) {
            if (!currentLine.empty()) {
                lines.push_back(currentLine);
                currentLine.clear();
            }
            lines.push_back("");
            continue;
        }

        // Word wrap the paragraph
        size_t paraPos = 0;
        while (paraPos < paragraph.size()) {
            // Skip leading spaces
            while (paraPos < paragraph.size() && paragraph[paraPos] == ' ') paraPos++;
            if (paraPos >= paragraph.size()) break;

            // Find word end
            size_t wordEnd = paragraph.find(' ', paraPos);
            if (wordEnd == std::string::npos) wordEnd = paragraph.size();

            std::string word = paragraph.substr(paraPos, wordEnd - paraPos);

            if (currentLine.empty()) {
                currentLine = word;
            } else if (currentLine.size() + 1 + word.size() <= (size_t)charsPerLine) {
                currentLine += " " + word;
            } else {
                lines.push_back(currentLine);
                currentLine = word;
            }

            paraPos = wordEnd;
        }
    }

    if (!currentLine.empty()) {
        lines.push_back(currentLine);
    }

    return lines;
}

// Find href by id in manifest
static std::string findHrefById(const std::vector<std::string>& manifest, const std::string& id) {
    for (const auto& item : manifest) {
        size_t pipePos = item.find('|');
        if (pipePos != std::string::npos) {
            std::string itemId = item.substr(0, pipePos);
            std::string itemHref = item.substr(pipePos + 1);
            if (itemId == id) return itemHref;
        }
    }
    return "";
}

extern "C" {

JNIEXPORT jstring JNICALL
Java_com_crosspoint_android_MainActivity_nativeGetVersion(JNIEnv* env, jobject /* this */) {
    return env->NewStringUTF("CrossPoint Android 0.8.0");
}

JNIEXPORT jstring JNICALL
Java_com_crosspoint_android_MainActivity_nativeGetLibraryInfo(JNIEnv* env, jobject /* this */) {
    return env->NewStringUTF(
        "CrossPoint Android NDK Build\n"
        "  - uzlib: compiled\n"
        "  - ZipFile: compiled\n"
        "  - Expat XML: compiled\n"
        "  - JsonParser: compiled"
    );
}

JNIEXPORT jboolean JNICALL
Java_com_crosspoint_android_MainActivity_nativeLoadEpub(JNIEnv* env, jobject /* this */,
                                                         jstring filepath, jstring cacheDir) {
    const char* fp = env->GetStringUTFChars(filepath, nullptr);
    const char* cd = env->GetStringUTFChars(cacheDir, nullptr);

    if (!fp || !cd) {
        LOGE("Failed to get string arguments");
        if (fp) env->ReleaseStringUTFChars(filepath, fp);
        if (cd) env->ReleaseStringUTFChars(cacheDir, cd);
        return JNI_FALSE;
    }

    LOGI("Loading EPUB: %s", fp);
    gEpubPath = fp;

    // Clear previous state
    gContentOpfPath.clear();
    gContentBasePath.clear();
    gSpineItems.clear();
    gTitle.clear();
    gAuthor.clear();
    gChapterCount = 0;

    // Parse container.xml to find content.opf
    std::string containerXml;
    if (readZipFileToString(gEpubPath, "META-INF/container.xml", containerXml)) {
        XML_Parser parser = XML_ParserCreate(nullptr);
        std::string contentOpfFile;

        XML_SetUserData(parser, &contentOpfFile);
        XML_SetElementHandler(parser,
            [](void* userData, const XML_Char* name, const XML_Char** attrs) {
                auto* result = static_cast<std::string*>(userData);
                // Handle namespaced tags like "opf:rootfile" or just "rootfile"
                const char* localName = strrchr(name, ':');
                if (localName) localName++;
                else localName = name;
                
                if (strcmp(localName, "rootfile") == 0) {
                    for (int i = 0; attrs[i] && attrs[i + 1]; i += 2) {
                        if (strcmp(attrs[i], "full-path") == 0) {
                            *result = attrs[i + 1];
                        }
                    }
                }
            }, nullptr);

        XML_Parse(parser, containerXml.c_str(), containerXml.size(), true);
        XML_ParserFree(parser);

        if (!contentOpfFile.empty()) {
            gContentOpfPath = contentOpfFile;
            gContentBasePath = contentOpfFile.substr(0, contentOpfFile.find_last_of('/') + 1);
            LOGI("Found content.opf: %s", gContentOpfPath.c_str());
        }
    }

    // Parse content.opf
    if (!gContentOpfPath.empty()) {
        std::string opfXml;
        if (readZipFileToString(gEpubPath, gContentOpfPath, opfXml)) {
            OpfParserState opfState;
            XML_Parser parser = XML_ParserCreate(nullptr);
            XML_SetUserData(parser, &opfState);
            XML_SetElementHandler(parser, opfStartElement, opfEndElement);
            XML_SetCharacterDataHandler(parser, opfCharacterData);
            XML_Parse(parser, opfXml.c_str(), opfXml.size(), true);
            XML_ParserFree(parser);

            gTitle = opfState.title;
            gAuthor = opfState.creator;

            // Build spine from manifest and spine refs
            for (const auto& ref : opfState.spineRefs) {
                std::string href = findHrefById(opfState.manifestItems, ref);
                if (!href.empty()) {
                    // Resolve relative path
                    std::string fullPath = gContentBasePath + href;
                    gSpineItems.push_back(fullPath);
                }
            }

            gChapterCount = gSpineItems.size();
            LOGI("Title: %s, Author: %s, Chapters: %d", gTitle.c_str(), gAuthor.c_str(), gChapterCount);
        }
    }

    env->ReleaseStringUTFChars(filepath, fp);
    env->ReleaseStringUTFChars(cacheDir, cd);

    return gChapterCount > 0 ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jint JNICALL
Java_com_crosspoint_android_MainActivity_nativeGetChapterCount(JNIEnv* env, jobject /* this */) {
    return gChapterCount;
}

JNIEXPORT jobjectArray JNICALL
Java_com_crosspoint_android_MainActivity_nativeGetChapterLines(JNIEnv* env, jobject /* this */,
                                                                jint chapterIndex) {
    if (chapterIndex < 0 || chapterIndex >= gChapterCount || gEpubPath.empty()) {
        LOGE("Invalid chapter index: %d", chapterIndex);
        return nullptr;
    }

    std::string chapterPath = gSpineItems[chapterIndex];
    LOGI("Loading chapter %d: %s", chapterIndex, chapterPath.c_str());

    std::string chapterHtml;
    if (!readZipFileToString(gEpubPath, chapterPath, chapterHtml)) {
        LOGE("Failed to read chapter: %s", chapterPath.c_str());
        return nullptr;
    }

    // Parse HTML to extract text
    HtmlToTextState htmlState;
    XML_Parser parser = XML_ParserCreate(nullptr);
    XML_SetUserData(parser, &htmlState);
    XML_SetElementHandler(parser, htmlStartElement, htmlEndElement);
    XML_SetCharacterDataHandler(parser, htmlCharacterData);
    XML_Parse(parser, chapterHtml.c_str(), chapterHtml.size(), true);
    XML_ParserFree(parser);

    LOGI("Chapter %d: %zu chars extracted", chapterIndex, htmlState.text.size());

    // Log first 200 chars for debugging
    std::string preview = htmlState.text.substr(0, 200);
    LOGI("Chapter %d preview: %s", chapterIndex, preview.c_str());

    // Word wrap text
    std::vector<std::string> lines = wrapText(htmlState.text, 110);
    LOGI("Chapter %d: %zu lines after wrapping", chapterIndex, lines.size());

    // Convert to Java String array
    jobjectArray result = env->NewObjectArray(lines.size(), env->FindClass("java/lang/String"), nullptr);
    for (size_t i = 0; i < lines.size(); i++) {
        jstring jstr = env->NewStringUTF(lines[i].c_str());
        env->SetObjectArrayElement(result, i, jstr);
        env->DeleteLocalRef(jstr);
    }

    return result;
}

JNIEXPORT jstring JNICALL
Java_com_crosspoint_android_MainActivity_nativeGetChapterTitle(JNIEnv* env, jobject /* this */,
                                                                jint chapterIndex) {
    if (chapterIndex < 0 || chapterIndex >= gChapterCount || gEpubPath.empty()) {
        LOGE("Invalid chapter index: %d", chapterIndex);
        return nullptr;
    }

    std::string chapterPath = gSpineItems[chapterIndex];
    LOGI("Getting title for chapter %d: %s", chapterIndex, chapterPath.c_str());

    std::string chapterHtml;
    if (!readZipFileToString(gEpubPath, chapterPath, chapterHtml)) {
        LOGE("Failed to read chapter: %s", chapterPath.c_str());
        return nullptr;
    }

    // Parse HTML to extract first heading
    HtmlToTextState htmlState;
    XML_Parser parser = XML_ParserCreate(nullptr);
    XML_SetUserData(parser, &htmlState);
    XML_SetElementHandler(parser, htmlStartElement, htmlEndElement);
    XML_SetCharacterDataHandler(parser, htmlCharacterData);
    XML_Parse(parser, chapterHtml.c_str(), chapterHtml.size(), true);
    XML_ParserFree(parser);

    // Use title if found, otherwise use first line of text or fallback
    std::string title;
    if (!htmlState.title.empty()) {
        title = htmlState.title;
    } else {
        // Extract first line from text content
        size_t newlinePos = htmlState.text.find('\n');
        if (newlinePos != std::string::npos && newlinePos < 100) {
            title = htmlState.text.substr(0, newlinePos);
        } else {
            title = "Chapter " + std::to_string(chapterIndex);
        }
    }

    // Trim whitespace
    size_t start = title.find_first_not_of(" \t\n\r");
    if (start != std::string::npos) {
        size_t end = title.find_last_not_of(" \t\n\r");
        title = title.substr(start, end - start + 1);
    }

    LOGI("Chapter %d title: %s", chapterIndex, title.c_str());
    return env->NewStringUTF(title.c_str());
}

JNIEXPORT jobject JNICALL
Java_com_crosspoint_android_MainActivity_nativeRenderPage(JNIEnv* env, jobject /* this */,
                                                           jobject bitmap, jint chapterIndex,
                                                           jfloat scrollPercent) {
    AndroidBitmapInfo info;
    void* pixels;

    int ret = AndroidBitmap_getInfo(env, bitmap, &info);
    if (ret != ANDROID_BITMAP_RESULT_SUCCESS) {
        LOGE("AndroidBitmap_getInfo failed: %d", ret);
        return nullptr;
    }

    ret = AndroidBitmap_lockPixels(env, bitmap, &pixels);
    if (ret != ANDROID_BITMAP_RESULT_SUCCESS) {
        LOGE("AndroidBitmap_lockPixels failed: %d", ret);
        return nullptr;
    }

    LOGI("Rendering to bitmap: %dx%d, stride=%d", info.width, info.height, info.stride);

    if (info.stride == (uint32_t)(info.width * 2)) {
        memset(pixels, 0xFF, info.stride * info.height);
    }

    AndroidBitmap_unlockPixels(env, bitmap);

    return bitmap;
}

JNIEXPORT jint JNICALL
Java_com_crosspoint_android_MainActivity_nativeTriggerEinkRefresh(JNIEnv* env, jobject /* this */,
                                                                   jint mode) {
    // Placeholder for native E-ink refresh
    // On Allwinner devices, this would typically involve:
    // 1. Opening /dev/iep or /dev/fb0
    // 2. Sending IOCTL command for E-ink update
    // 3. Waiting for completion
    
    LOGI("Native E-ink refresh requested with mode: %d", mode);
    LOGI("Note: Native refresh not implemented yet. Using Java reflection/broadcasts.");
    
    return 0; // Success (placeholder)
}

} // extern "C"