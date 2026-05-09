#pragma once
#include <cstdint>
class HalGPIO {
public:
    enum Button { BTN_UP = 0, BTN_DOWN = 1, BTN_BACK = 2, BTN_CONFIRM = 3 };
    static HalGPIO& getInstance() { static HalGPIO inst; return inst; }
    void init() {}
    bool update() { return false; }
    bool wasPressed(Button) { return false; }
    bool isPressed(Button) { return false; }
    bool wasLongPressed(Button) { return false; }
};

#define GPIO HalGPIO::getInstance()