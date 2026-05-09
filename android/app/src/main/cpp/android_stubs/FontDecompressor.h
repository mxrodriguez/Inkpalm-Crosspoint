#pragma once

#include <InflateReader.h>

#include <vector>

#include "EpdFontData.h"

class FontDecompressor {
 public:
  static constexpr uint16_t MAX_PAGE_GLYPHS = 512;
  static constexpr uint8_t MAX_PAGE_SLOTS = 4;

  FontDecompressor() = default;
  ~FontDecompressor();

  bool init();
  void deinit();

  const uint8_t* getBitmap(const EpdFontData* fontData, const EpdGlyph* glyph, uint32_t glyphIndex);

  void clearCache();

  int prewarmCache(const EpdFontData* fontData, const char* utf8Text);

  struct Stats {
    uint32_t cacheHits = 0;
    uint32_t cacheMisses = 0;
    uint32_t decompressTimeMs = 0;
    uint16_t uniqueGroupsAccessed = 0;
    uint32_t pageBufferBytes = 0;
    uint32_t pageGlyphsBytes = 0;
    uint32_t hotGroupBytes = 0;
    uint32_t peakTempBytes = 0;
    uint32_t getBitmapTimeUs = 0;
    uint32_t getBitmapCalls = 0;
  };
  void logStats(const char* label = "FDC");
  void resetStats();
  const Stats& getStats() const { return stats; }

 private:
  Stats stats;
  InflateReader inflateReader;

  struct PageGlyphEntry {
    uint32_t glyphIndex;
    uint32_t bufferOffset;
    uint32_t alignedOffset;
  };
  struct PageSlot {
    uint8_t* buffer = nullptr;
    const EpdFontData* fontData = nullptr;
    PageGlyphEntry* glyphs = nullptr;
    uint16_t glyphCount = 0;
  };
  PageSlot pageSlots[MAX_PAGE_SLOTS] = {};
  uint8_t pageSlotCount = 0;

  const EpdFontData* hotGroupFont = nullptr;
  uint16_t hotGroupIndex = UINT16_MAX;
  std::vector<uint8_t> hotGroup;

  std::vector<uint8_t> hotGlyphBuf;

  void freePageBuffer();
  void freeHotGroup();
  uint16_t getGroupIndex(const EpdFontData* fontData, uint32_t glyphIndex);
  uint32_t getAlignedOffset(const EpdFontData* fontData, uint16_t groupIndex, uint32_t glyphIndex);
  bool decompressGroup(const EpdFontData* fontData, uint16_t groupIndex, uint8_t* outBuf, uint32_t outSize);
  static void compactSingleGlyph(const uint8_t* alignedSrc, uint8_t* packedDst, uint8_t width, uint8_t height);
  static int32_t findGlyphIndex(const EpdFontData* fontData, uint32_t codepoint);
};
