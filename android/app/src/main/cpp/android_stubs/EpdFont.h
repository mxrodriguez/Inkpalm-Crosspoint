#pragma once
#include "EpdFontData.h"

class EpdFont {
  void getTextBounds(const char* string, int startX, int startY, int* minX, int* minY, int* maxX, int* maxY) const;

 public:
  const EpdFontData* data;
  explicit EpdFont(const EpdFontData* data) : data(data) {}
  ~EpdFont() = default;
  void getTextDimensions(const char* string, int* w, int* h) const;

  const EpdGlyph* getGlyph(uint32_t cp) const;

  /// Returns the kerning adjustment (4.4 fixed-point in pixels) between two codepoints.
  int8_t getKerning(uint32_t leftCp, uint32_t rightCp) const;

  /// Returns the ligature codepoint for a pair, or 0 if no ligature exists.
  uint32_t getLigature(uint32_t leftCp, uint32_t rightCp) const;

  /// Greedily applies ligature substitutions starting from cp.
  uint32_t applyLigatures(uint32_t cp, const char*& text) const;
};
