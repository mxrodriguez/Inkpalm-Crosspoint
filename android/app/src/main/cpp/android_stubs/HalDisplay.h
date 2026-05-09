#pragma once
#include "Arduino.h"
#include <cstdint>
#include <cstdlib>
#include <cstring>

class HalDisplay {
 public:
  // Refresh modes — matches GfxRenderer's expected API
  enum RefreshMode {
    FULL_REFRESH,   // Full refresh with complete waveform
    HALF_REFRESH,   // Half refresh - balanced quality and speed
    FAST_REFRESH    // Fast refresh using custom LUT
  };

  HalDisplay() = default;
  ~HalDisplay() { free(frameBuffer_); free(lsbBuffer_); free(msbBuffer_); }

  // InkPalm 5: 720×1280 portrait panel
  // The framebuffer is 1bpp MSB-first, bit=0 is BLACK (same as ESP32 original)
  static constexpr uint16_t DISPLAY_WIDTH = 1280;
  static constexpr uint16_t DISPLAY_HEIGHT = 720;
  static constexpr uint16_t DISPLAY_WIDTH_BYTES = DISPLAY_WIDTH / 8;
  static constexpr uint32_t BUFFER_SIZE = DISPLAY_WIDTH_BYTES * DISPLAY_HEIGHT;

  void begin() {
    if (!frameBuffer_) {
      frameBuffer_ = static_cast<uint8_t*>(calloc(1, BUFFER_SIZE));
    }
  }

  // Frame buffer operations
  void clearScreen(uint8_t color = 0xFF) const {
    if (frameBuffer_) memset(frameBuffer_, color, BUFFER_SIZE);
  }

  // Draw a 1bpp image directly into the framebuffer (used by GfxRenderer::drawImage)
  void drawImage(const uint8_t* imageData, uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                 bool fromProgmem = false) const {
    // Simple 1bpp blit — copy each row of the image into the framebuffer
    if (!frameBuffer_ || !imageData) return;
    const uint16_t srcWidthBytes = (w + 7) / 8;
    for (uint16_t row = 0; row < h; row++) {
      if (y + row >= DISPLAY_HEIGHT) break;
      for (uint16_t col = 0; col < w; col++) {
        if (x + col >= DISPLAY_WIDTH) break;
        uint16_t srcByteIdx = row * srcWidthBytes + (col / 8);
        uint8_t srcBit = 7 - (col % 8);
        bool pixel = (imageData[srcByteIdx] >> srcBit) & 1;
        // In our framebuffer: bit=0 is BLACK, bit=1 is WHITE
        // Source: bit=1 means pixel is set (black in e-ink terms)
        uint32_t dstByteIdx = static_cast<uint32_t>(y + row) * DISPLAY_WIDTH_BYTES + ((x + col) / 8);
        uint8_t dstBit = 7 - ((x + col) % 8);
        if (pixel) {
          frameBuffer_[dstByteIdx] &= ~(1 << dstBit);  // Clear bit = BLACK
        } else {
          frameBuffer_[dstByteIdx] |= (1 << dstBit);   // Set bit = WHITE
        }
      }
    }
  }

  // Draw a 1bpp image with transparency (white pixels are skipped)
  void drawImageTransparent(const uint8_t* imageData, uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                            bool fromProgmem = false) const {
    if (!frameBuffer_ || !imageData) return;
    const uint16_t srcWidthBytes = (w + 7) / 8;
    for (uint16_t row = 0; row < h; row++) {
      if (y + row >= DISPLAY_HEIGHT) break;
      for (uint16_t col = 0; col < w; col++) {
        if (x + col >= DISPLAY_WIDTH) break;
        uint16_t srcByteIdx = row * srcWidthBytes + (col / 8);
        uint8_t srcBit = 7 - (col % 8);
        bool pixel = (imageData[srcByteIdx] >> srcBit) & 1;
        if (!pixel) continue;  // Skip white (transparent) pixels
        uint32_t dstByteIdx = static_cast<uint32_t>(y + row) * DISPLAY_WIDTH_BYTES + ((x + col) / 8);
        uint8_t dstBit = 7 - ((x + col) % 8);
        frameBuffer_[dstByteIdx] &= ~(1 << dstBit);  // Clear bit = BLACK
      }
    }
  }

  void displayBuffer(RefreshMode mode = FAST_REFRESH, bool fadingFix = false) {
    // On Android, the JNI layer will read the framebuffer and push it to the e-ink display
    // via the Moaan broadcast intent. This is a placeholder — the actual JNI call
    // will be implemented in Step 3 (E-ink hardware control).
    refreshPending_ = true;
    lastRefreshMode_ = mode;
  }

  void refreshDisplay(RefreshMode mode = FAST_REFRESH, bool fadingFix = false) {
    displayBuffer(mode, fadingFix);
  }

  // Access to frame buffer
  uint8_t* getFrameBuffer() const { return frameBuffer_; }

  // Grayscale buffer operations (for 4-level gray rendering)
  void copyGrayscaleBuffers(const uint8_t* lsbBuffer, const uint8_t* msbBuffer) {
    if (!lsbBuffer_) lsbBuffer_ = static_cast<uint8_t*>(calloc(1, BUFFER_SIZE));
    if (!msbBuffer_) msbBuffer_ = static_cast<uint8_t*>(calloc(1, BUFFER_SIZE));
    if (lsbBuffer_ && lsbBuffer) memcpy(lsbBuffer_, lsbBuffer, BUFFER_SIZE);
    if (msbBuffer_ && msbBuffer) memcpy(msbBuffer_, msbBuffer, BUFFER_SIZE);
  }

  void copyGrayscaleLsbBuffers(const uint8_t* srcBuffer) {
    if (!lsbBuffer_) lsbBuffer_ = static_cast<uint8_t*>(calloc(1, BUFFER_SIZE));
    if (lsbBuffer_ && srcBuffer) memcpy(lsbBuffer_, srcBuffer, BUFFER_SIZE);
  }

  void copyGrayscaleMsbBuffers(const uint8_t* srcBuffer) {
    if (!msbBuffer_) msbBuffer_ = static_cast<uint8_t*>(calloc(1, BUFFER_SIZE));
    if (msbBuffer_ && srcBuffer) memcpy(msbBuffer_, srcBuffer, BUFFER_SIZE);
  }

  void cleanupGrayscaleBuffers(const uint8_t* bwBuffer) {
    // Restore the BW buffer from the provided source after grayscale rendering
    if (frameBuffer_ && bwBuffer) memcpy(frameBuffer_, bwBuffer, BUFFER_SIZE);
  }

  void displayGrayBuffer(bool fadingFix = false) {
    // Composite LSB + MSB buffers into framebuffer, then display
    // This will be implemented properly in Step 3
    refreshPending_ = true;
    lastRefreshMode_ = FULL_REFRESH;
  }

  // Runtime geometry passthrough
  uint16_t getDisplayWidth() const { return DISPLAY_WIDTH; }
  uint16_t getDisplayHeight() const { return DISPLAY_HEIGHT; }
  uint16_t getDisplayWidthBytes() const { return DISPLAY_WIDTH_BYTES; }
  uint32_t getBufferSize() const { return BUFFER_SIZE; }

  // Power management — no-op on Android
  void deepSleep() {}

  // Android-specific: check if a refresh is pending (for JNI layer to poll)
  bool isRefreshPending() const { return refreshPending_; }
  void clearRefreshPending() { refreshPending_ = false; }
  RefreshMode getLastRefreshMode() const { return lastRefreshMode_; }

 private:
  uint8_t* frameBuffer_ = nullptr;
  uint8_t* lsbBuffer_ = nullptr;
  uint8_t* msbBuffer_ = nullptr;
  bool refreshPending_ = false;
  RefreshMode lastRefreshMode_ = FAST_REFRESH;
};

extern HalDisplay display;
