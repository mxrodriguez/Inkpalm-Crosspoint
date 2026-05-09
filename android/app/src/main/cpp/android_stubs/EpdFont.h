#pragma once
#include <cstdint>
#include <string>
#include <vector>

class EpdFontData;
class EpdGlyph;

class EpdFontFamily;

class EpdFont {
public:
    enum Style { REGULAR = 0, BOLD = 1, ITALIC = 2, BOLD_ITALIC = 3 };
    static const uint16_t INVALID_GLYPH = 0xFFFF;

    explicit EpdFont(const uint8_t* data);
    ~EpdFont();
    const EpdGlyph* getGlyph(uint32_t cp) const;
    void getTextDimensions(const char* text, int* w, int* h, Style style = REGULAR) const;
    int getGlyphAdvance(uint32_t cp) const;
    int8_t getKerning(uint32_t leftCp, uint32_t rightCp) const;
    bool hasStyle(Style) const { return true; }
    uint32_t applyLigatures(uint32_t cp, const char*& text) const { return cp; }
    const EpdFontData* getData() const { return nullptr; }
};