#pragma once
#include "WString.h"

class HalDisplay {
public:
    static constexpr int DISPLAY_WIDTH = 1280;
    static constexpr int DISPLAY_HEIGHT = 720;
    static constexpr int DISPLAY_WIDTH_BYTES = DISPLAY_WIDTH / 8;
    static constexpr size_t BUFFER_SIZE = DISPLAY_WIDTH_BYTES * DISPLAY_HEIGHT;
    static constexpr size_t GRAYSCALE_BUFFER_SIZE = DISPLAY_WIDTH * DISPLAY_HEIGHT;

    static HalDisplay& getInstance() { static HalDisplay inst; return inst; }
    void init() {}
    void clear() {}
    void update() {}
    void update(int x, int y, int w, int h) {}
    void fill(uint8_t color) {}
    void drawPixel(int x, int y, uint8_t color) {}
    void fillRect(int x, int y, int w, int h, uint8_t color) {}
    uint8_t* getFramebuffer() { return nullptr; }
    uint8_t* getGrayscaleFramebuffer() { return nullptr; }
    void partialUpdate(int x, int y, int w, int h) {}
    void fullRefresh() {}
};

#define Display HalDisplay::getInstance()