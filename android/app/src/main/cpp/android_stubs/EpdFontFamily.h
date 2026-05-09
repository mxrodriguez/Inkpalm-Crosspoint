#pragma once
#include "EpdFont.h"

class EpdFontFamily {
public:
    enum Style { REGULAR = 0, BOLD = 1, ITALIC = 2, BOLD_ITALIC = 3 };
    explicit EpdFontFamily(const EpdFont* regular = nullptr, const EpdFont* bold = nullptr,
                           const EpdFont* italic = nullptr, const EpdFont* boldItalic = nullptr) {}
    void getTextDimensions(const char*, int*, int*, Style = REGULAR) const {}
    const EpdFontData* getData(Style = REGULAR) const { return nullptr; }
    const EpdGlyph* getGlyph(uint32_t cp, Style = REGULAR) const { return nullptr; }
    int8_t getKerning(uint32_t, uint32_t, Style = REGULAR) const { return 0; }
    uint32_t applyLigatures(uint32_t cp, const char*& text, Style = REGULAR) const { return cp; }
};